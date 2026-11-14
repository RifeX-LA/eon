#include "test.hpp"
#include <deque>
#include <eon/asio.hpp>
#include <forward_list>
#include <future>
#include <list>
#include <map>
#include <set>

namespace net = eon::asio;
using Tcp = boost::asio::ip::tcp;

namespace serialization_example {
struct Member {
    int value{};

    void write(Tcp::socket & socket) const {
        net::write(socket, value);
    }

    void read(Tcp::socket & socket) {
        value = net::read<int>(socket);
    }

    bool operator==(Member const &) const = default;
};

struct Adl {
    int value{};
    bool operator==(Adl const &) const = default;
};

void write(Tcp::socket & socket, Adl const & value) {
    net::write(socket, value.value);
}

void read(Tcp::socket & socket, Adl & value) {
    value.value = net::read<int>(socket);
}

struct Both : Member {};

void write(Tcp::socket &, Both const &) {
    throw std::logic_error("ADL must not override members");
}

void read(Tcp::socket &, Both &) {
    throw std::logic_error("ADL must not override members");
}

struct NonConstWriter {
    void write(Tcp::socket &);
    void read(Tcp::socket &);
};

struct ThrowingMove {
    ThrowingMove() = default;

    ThrowingMove(ThrowingMove &&) {
        throw std::runtime_error("move");
    }

    ThrowingMove & operator=(ThrowingMove &&) = default;

    void write(Tcp::socket & socket) const {
        net::write(socket, 0);
    }

    void read(Tcp::socket & socket) {
        (void)net::read<int>(socket);
    }
};
} // namespace serialization_example

static_assert(net::detail::endian_reversable<int>);
static_assert(!net::detail::endian_reversable<bool>);
static_assert(!net::detail::endian_reversable<double>);
static_assert(net::detail::asio_serializer<net::detail::integral_serializer<int>>);
static_assert(net::detail::has_write_member<serialization_example::Member>);
static_assert(net::detail::has_read_member<serialization_example::Member>);
static_assert(!net::detail::has_write_member<serialization_example::NonConstWriter>);
static_assert(net::detail::has_write_adl<serialization_example::Adl>);
static_assert(net::detail::has_read_adl<serialization_example::Adl>);
static_assert(std::same_as<decltype(net::detail::serializer_for<bool>()), net::detail::bool_serializer>);
static_assert(std::same_as<decltype(net::detail::serializer_for<int>()), net::detail::integral_serializer<int>>);
static_assert(std::same_as<decltype(net::detail::serializer_for<serialization_example::Both>()),
                           net::detail::member_serializer<serialization_example::Both>>);
static_assert(std::same_as<decltype(net::detail::serializer_for<serialization_example::Adl>()),
                           net::detail::adl_serializer<serialization_example::Adl>>);
static_assert(noexcept(net::detail::serializer_for<int>()));

class AsioTest : public ::testing::Test {
protected:
    boost::asio::io_context io;
    Tcp::acceptor acceptor{io, {boost::asio::ip::address_v4::loopback(), 0}};
    Tcp::socket sender{io}, receiver{io};

    void SetUp() override {
        sender.connect(acceptor.local_endpoint());
        acceptor.accept(receiver);
        acceptor.close();
    }

    template <typename Write, class Read>
    auto transfer(Write write, Read read) {
        auto future = std::async(std::launch::async, [&] {
            try {
                write();
                sender.shutdown(Tcp::socket::shutdown_send);
            } catch (...) {
                boost::system::error_code ignored;
                sender.close(ignored);
                throw;
            }
        });
        try {
            if constexpr (std::is_void_v<std::invoke_result_t<Read>>) {
                read();
                future.get();
            } else {
                auto result = read();
                future.get();
                return result;
            }
        } catch (...) {
            boost::system::error_code ignored;
            receiver.close(ignored);
            future.wait();
            throw;
        }
    }

    template <typename T>
    T round_trip(T const & value) {
        constexpr std::uint32_t marker = 0x12345678;
        return transfer(
            [&] {
                net::write(sender, value);
                net::write(sender, marker);
            },
            [&] {
                T result = net::read<T>(receiver);
                EXPECT_EQ(net::read<std::uint32_t>(receiver), marker);
                return result;
            });
    }
};

template <typename T>
class AsioIntegerTest : public AsioTest {};

using Integers = ::testing::Types<signed char, unsigned char, short, unsigned short, int, unsigned, long, unsigned long,
                                  long long, unsigned long long>;
TYPED_TEST_SUITE(AsioIntegerTest, Integers);

TYPED_TEST(AsioIntegerTest, MinimumMaximumZeroAndOne) {
    using T = TypeParam;
    // A fresh socket pair per exchange: round_trip closes the write direction.
    for (T value : {std::numeric_limits<T>::lowest(), T{}, T{1}, std::numeric_limits<T>::max()}) {
        net::write(this->sender, value);
        EXPECT_EQ(net::read<T>(this->receiver), value);
    }
}

TEST_F(AsioTest, IntegerWireFormatIsBigEndian) {
    net::write(sender, std::uint32_t{0x01020304});
    std::array<unsigned char, 4> bytes{};
    boost::asio::read(receiver, boost::asio::buffer(bytes));
    EXPECT_EQ(bytes, (std::array<unsigned char, 4>{1, 2, 3, 4}));
    net::write(sender, std::int32_t{-2});
    boost::asio::read(receiver, boost::asio::buffer(bytes));
    EXPECT_EQ(bytes, (std::array<unsigned char, 4>{255, 255, 255, 254}));
}

TEST_F(AsioTest, BooleanValues) {
    for (bool value : {false, true}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<bool>(receiver), value);
    }
}

TEST_F(AsioTest, EnumUnderlyingBoundaries) {
    enum class Code : std::int16_t {
        min = std::numeric_limits<std::int16_t>::min(),
        zero = 0,
        max = std::numeric_limits<std::int16_t>::max()
    };
    for (auto value : {Code::min, Code::zero, Code::max}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<Code>(receiver), value);
    }
}

TEST_F(AsioTest, MemberCustomization) {
    serialization_example::Member member{42};
    net::write(sender, member);
    EXPECT_EQ(net::read<serialization_example::Member>(receiver), member);
}

TEST_F(AsioTest, AdlCustomization) {
    serialization_example::Adl adl{17};
    net::write(sender, adl);
    EXPECT_EQ(net::read<serialization_example::Adl>(receiver), adl);
}

TEST_F(AsioTest, MembersTakePriorityOverAdl) {
    serialization_example::Both both;
    both.value = 9;
    net::write(sender, both);
    EXPECT_EQ(net::read<serialization_example::Both>(receiver).value, 9);
}

TEST_F(AsioTest, HeterogeneousTupleHasLeftToRightWireOrder) {
    const auto value = std::tuple{std::int16_t{-7}, std::uint32_t{12345}, std::string("hello"), true};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, EmptyTupleWritesNoElements) {
    net::write(sender, std::tuple{});
    EXPECT_EQ(net::read<std::tuple<>>(receiver), std::tuple{});
}

TEST_F(AsioTest, NestedPairRoundTrip) {
    const auto value = std::pair{42, std::tuple{std::string("nested"), -1}};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, ExpectedSuccessAndError) {
    using Result = std::expected<int, std::string>;
    for (Result const & value : {Result{42}, Result{std::unexpected("error")}}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<Result>(receiver), value);
    }
}

TEST_F(AsioTest, OptionalEmptyAndPresent) {
    for (std::optional<int> value : {std::optional<int>{}, std::optional<int>{0}, std::optional<int>{-1}}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<std::optional<int>>(receiver), value);
    }
}

TEST_F(AsioTest, ExpectedVoidSuccessAndError) {
    using VoidResult = std::expected<void, std::string>;
    for (VoidResult const & value : {VoidResult{}, VoidResult{std::unexpected("error")}}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<VoidResult>(receiver), value);
    }
}

TEST_F(AsioTest, NestedOptionalStates) {
    for (auto value :
         {std::optional<std::optional<int>>{}, std::optional<std::optional<int>>{std::in_place, std::nullopt},
          std::optional<std::optional<int>>{std::in_place, 7}}) {
        net::write(sender, value);
        EXPECT_EQ(net::read<decltype(value)>(receiver), value);
    }
}

TEST_F(AsioTest, UniquePointerOwnsDeserializedValue) {
    auto unique = std::make_unique<int>(9);
    net::write(sender, unique);
    auto copy = net::read<std::unique_ptr<int>>(receiver);
    ASSERT_NE(copy, nullptr);
    EXPECT_EQ(*copy, 9);
}

TEST_F(AsioTest, EmptyUniquePointerRoundTrip) {
    std::unique_ptr<int> unique;
    unique.reset();
    net::write(sender, unique);
    EXPECT_EQ(net::read<std::unique_ptr<int>>(receiver), nullptr);
}

TEST_F(AsioTest, SharedPointerOwnsDeserializedValue) {
    auto shared = std::make_shared<int>(11);
    net::write(sender, shared);
    auto shared_copy = net::read<std::shared_ptr<int>>(receiver);
    ASSERT_NE(shared_copy, nullptr);
    EXPECT_EQ(*shared_copy, 11);
}

TEST_F(AsioTest, VariantAlternativesAndDuplicateTypes) {
    using V = std::variant<int, std::string, int>;
    for (V value : {V{std::in_place_index<0>, 7}, V{std::in_place_index<1>, "text"}, V{std::in_place_index<2>, 9}}) {
        net::write(sender, value);
        auto result = net::read<V>(receiver);
        EXPECT_EQ(result.index(), value.index());
        EXPECT_EQ(result, value);
    }
}

TEST_F(AsioTest, VariantWithMoveOnlyAlternative) {
    using V = std::variant<int, std::unique_ptr<int>>;
    V value{std::in_place_index<1>, std::make_unique<int>(3)};
    auto result = round_trip(value);
    ASSERT_EQ(result.index(), 1);
    ASSERT_NE(std::get<1>(result), nullptr);
    EXPECT_EQ(*std::get<1>(result), 3);
}

TEST_F(AsioTest, InvalidVariantIndicesAreRejected) {
    using V = std::variant<int, std::string>;
    for (auto index : {std::size_t{2}, std::size_t{100}, std::variant_npos}) {
        net::write(sender, index);
        EXPECT_THROW((void)net::read<V>(receiver), std::bad_variant_access);
    }
}

TEST_F(AsioTest, ValuelessVariantIsRejectedBeforeWritingItsTag) {
    using V = std::variant<int, serialization_example::ThrowingMove>;
    V destination{0};
    V source{std::in_place_index<1>};
    EXPECT_THROW(destination = std::move(source), std::runtime_error);
    ASSERT_TRUE(destination.valueless_by_exception());
    EXPECT_THROW(net::write(sender, destination), std::bad_variant_access);
    net::write(sender, 42);
    EXPECT_EQ(net::read<int>(receiver), 42);
}

TEST_F(AsioTest, EmptyVectorRoundTrip) {
    const auto value = std::vector<int>{};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, IntegerVectorRoundTrip) {
    const auto value = std::vector<int>{1, -2, 3};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, DequeRoundTrip) {
    const auto value = std::deque<int>{4, 5};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, ListRoundTrip) {
    const auto value = std::list<int>{6, 7};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, ForwardListRoundTrip) {
    const auto value = std::forward_list<int>{8, 9};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, SetRoundTrip) {
    const auto value = std::set<int>{3, 1, 2};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, FixedArrayRoundTrip) {
    const auto value = std::array<int, 3>{1, 2, 3};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, EmptyFixedArrayRoundTrip) {
    const auto value = std::array<int, 0>{};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, BitVectorRoundTrip) {
    const auto value = std::vector<bool>{true, false, true};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, StringPreservesEmbeddedZero) {
    const std::string text("a\0b", 3);
    net::write(sender, text);
    EXPECT_EQ(net::read<std::string>(receiver), text);
}

TEST_F(AsioTest, WideStringRoundTrip) {
    const std::wstring wide = L"wide";
    net::write(sender, wide);
    EXPECT_EQ(net::read<std::wstring>(receiver), wide);
}

TEST_F(AsioTest, EmptyStringRoundTrip) {
    net::write(sender, std::string{});
    EXPECT_TRUE(net::read<std::string>(receiver).empty());
}

struct NonDefaultElement {
    const int value;
    NonDefaultElement() = delete;

    explicit NonDefaultElement(int value) : value(value) {
    }

    bool operator==(NonDefaultElement const &) const = default;
};

static_assert(std::is_trivially_copyable_v<NonDefaultElement>);
static_assert(!std::default_initializable<NonDefaultElement>);
static_assert(!std::is_copy_assignable_v<NonDefaultElement>);

TEST_F(AsioTest, VectorOfNonDefaultNonAssignableElements) {
    using Values = std::vector<NonDefaultElement>;
    const Values vector{NonDefaultElement{1}, NonDefaultElement{2}};
    net::write(sender, vector);
    EXPECT_EQ(net::read<Values>(receiver), vector);
}

TEST_F(AsioTest, ForwardListOfNonDefaultNonAssignableElements) {
    using Forward = std::forward_list<NonDefaultElement>;
    const Forward forward{NonDefaultElement{3}, NonDefaultElement{4}};
    net::write(sender, forward);
    EXPECT_EQ(net::read<Forward>(receiver), forward);
}

TEST_F(AsioTest, ContainersOfMoveOnlyValues) {
    std::vector<std::unique_ptr<int>> values;
    values.push_back(std::make_unique<int>(17));
    values.push_back(nullptr);
    auto copy = round_trip(values);
    ASSERT_EQ(copy.size(), 2);
    ASSERT_NE(copy[0], nullptr);
    EXPECT_EQ(*copy[0], 17);
    EXPECT_EQ(copy[1], nullptr);
}

TEST_F(AsioTest, AssociativeRangesPreserveKeysAndValues) {
    const std::map<int, std::string> value{{1, "one"}, {2, "two"}};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, FloatingRangesTransferAllBytesAndPreserveFraming) {
    const std::vector<double> value{
        0., -0., 1.5, -2., std::numeric_limits<double>::max(), std::numeric_limits<double>::denorm_min()};
    auto result = round_trip(value);
    EXPECT_EQ(result, value);
    EXPECT_TRUE(std::signbit(result[1]));
}

TEST_F(AsioTest, FixedFloatingArrayRoundTrip) {
    const std::array<float, 3> value{1.25f, -2.5f, 3.75f};
    EXPECT_EQ(round_trip(value), value);
}

TEST_F(AsioTest, FixedArrayRejectsIncorrectLength) {
    for (std::size_t size : {std::size_t{0}, std::size_t{4}, std::numeric_limits<std::size_t>::max()}) {
        net::write(sender, size);
        EXPECT_THROW((void)(net::read<std::array<int, 3>>(receiver)), std::length_error);
    }
}

TEST_F(AsioTest, DynamicRangeRejectsOverflowingLength) {
    net::write(sender, std::numeric_limits<std::size_t>::max());
    EXPECT_THROW((void)net::read<std::vector<double>>(receiver), std::length_error);
}

struct Raw {
    std::uint32_t first;
    std::uint32_t second;
    bool operator==(Raw const &) const = default;
};

static_assert(std::is_trivially_copyable_v<Raw>);

TEST_F(AsioTest, RawBufferReadReturnsValue) {
    Raw value{0x12345678, 0xabcdef01};
    net::write_as_buffer(sender, value);
    EXPECT_EQ(net::read_as_buffer<Raw>(receiver), value);
}

TEST_F(AsioTest, RawBufferReadUpdatesExistingObject) {
    Raw value{0x12345678, 0xabcdef01};
    net::write_as_buffer(sender, value);
    Raw result{};
    net::read_as_buffer(receiver, result);
    EXPECT_EQ(result, value);
}

TEST_F(AsioTest, DefaultSerializerUsesRawRepresentation) {
    Raw value{0x12345678, 0xabcdef01};
    net::write(sender, value);
    EXPECT_EQ(net::read<Raw>(receiver), value);
}

TEST_F(AsioTest, ScalarFloatingRepresentationIncludingSpecialValues) {
    for (double value : {0., -0., std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
        net::write(sender, value);
        auto result = net::read<double>(receiver);
        EXPECT_EQ(std::bit_cast<std::uint64_t>(result), std::bit_cast<std::uint64_t>(value));
    }
}

TEST_F(AsioTest, TruncatedIntegralAndRangeReportEof) {
    EXPECT_THROW(transfer(
                     [&] {
                         std::array<char, 1> data{'x'};
                         boost::asio::write(sender, boost::asio::buffer(data));
                     },
                     [&] { (void)net::read<std::uint64_t>(receiver); }),
                 boost::system::system_error);
}

TEST_F(AsioTest, TruncatedRangeReportsEof) {
    EXPECT_THROW(transfer(
                     [&] {
                         net::write(sender, std::size_t{3});
                         net::write(sender, 42);
                     },
                     [&] { (void)net::read<std::vector<int>>(receiver); }),
                 boost::system::system_error);
}

TEST(AsioErrors, ClosedSocketsFailWithoutBlocking) {
    boost::asio::io_context io;
    Tcp::socket socket(io);
    EXPECT_THROW(net::write(socket, 1), boost::system::system_error);
    EXPECT_THROW((void)net::read<int>(socket), boost::system::system_error);
}

class AsioFileTest : public AsioTest {
protected:
    std::filesystem::path directory;

    void SetUp() override {
        AsioTest::SetUp();
        auto base = std::filesystem::temp_directory_path();
        for (unsigned attempt = 0; attempt < 100; ++attempt) {
            auto candidate =
                base / ("eon-asio-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
                        "-" + std::to_string(attempt));
            if (std::filesystem::create_directory(candidate)) {
                directory = candidate;
                return;
            }
        }
        throw std::runtime_error("Cannot create isolated test directory");
    }

    void TearDown() override {
        if (!directory.empty()) {
            std::error_code ec;
            auto target = std::filesystem::weakly_canonical(directory, ec);
            ASSERT_FALSE(ec);
            auto root = std::filesystem::weakly_canonical(std::filesystem::temp_directory_path(), ec);
            ASSERT_FALSE(ec);
            ASSERT_EQ(target.parent_path(), root);
            ASSERT_TRUE(target.filename().string().starts_with("eon-asio-"));
            std::filesystem::remove_all(target, ec);
            EXPECT_FALSE(ec);
        }
    }
};

class AsioFileSizeTest : public AsioFileTest, public ::testing::WithParamInterface<std::size_t> {};

TEST_P(AsioFileSizeTest, BinaryRoundTripAcrossChunkBoundaries) {
    std::string bytes(GetParam(), '\0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<char>(i % 256);
    }
    auto source = directory / "source.bin", destination = directory / "destination.bin";
    {
        std::ofstream out(source, std::ios::binary);
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    }
    transfer([&] { net::write_file(sender, source); }, [&] { net::read_file(receiver, destination); });
    std::ifstream in(destination, std::ios::binary);
    std::string received{std::istreambuf_iterator<char>(in), {}};
    EXPECT_EQ(received, bytes);
}

INSTANTIATE_TEST_SUITE_P(ChunkSizes, AsioFileSizeTest,
                         ::testing::Values(std::size_t{0}, std::size_t{1}, net::detail::file_chunk_size - 1,
                                           net::detail::file_chunk_size, net::detail::file_chunk_size + 1,
                                           net::detail::file_chunk_size * 3 + 17));

TEST_F(AsioFileTest, MissingSourceIsReported) {
    EXPECT_THROW(net::write_file(sender, directory / "missing.bin"), std::ios_base::failure);
}

TEST_F(AsioFileTest, InvalidDestinationIsReported) {
    EXPECT_THROW(net::read_file(receiver, directory / "missing-directory" / "out.bin"), std::ios_base::failure);
}

static_assert(std::ranges::input_range<net::detail::deserialization_view<int>>);
static_assert(!std::ranges::forward_range<net::detail::deserialization_view<int>>);
static_assert(std::input_iterator<net::detail::deserialization_view<std::unique_ptr<int>>::iterator>);

TEST_F(AsioTest, InputViewCachesDereferencesAndDoesNotReadPastItsCount) {
    net::write(sender, 11);
    net::write(sender, 22);
    net::write(sender, 33);
    net::detail::deserialization_view<int> view(receiver, 2);
    auto it = view.begin();
    EXPECT_EQ(*it, 11);
    EXPECT_EQ(*it, 11);
    ++it;
    EXPECT_EQ(*it, 22);
    ++it;
    EXPECT_EQ(it, view.end());
    EXPECT_EQ(net::read<int>(receiver), 33);
    net::detail::deserialization_view<int> empty(receiver, 0);
    EXPECT_EQ(empty.begin(), empty.end());
}

template <typename Fn>
void expect_timeout(Fn fn) {
    try {
        fn();
        FAIL() << "Expected a timeout";
    } catch (boost::system::system_error const & error) {
        EXPECT_EQ(error.code(), boost::asio::error::timed_out);
    }
}

TEST_F(AsioTest, TimedReadAndWritePreserveSerializationAndBlockingMode) {
    using namespace std::chrono_literals;
    const auto value = std::tuple{42, std::string("timed"), std::vector<double>{1., 2.}};
    net::write_with_timeout(sender, value, 2s);
    EXPECT_EQ((net::read_with_timeout<decltype(value)>(receiver, 2s)), value);
    EXPECT_FALSE(sender.non_blocking());
    EXPECT_FALSE(receiver.non_blocking());
}

TEST_F(AsioTest, TimedReadWaitsForDelayedData) {
    using namespace std::chrono_literals;
    auto value = transfer(
        [&] {
            std::this_thread::sleep_for(20ms);
            net::write(sender, 42);
        },
        [&] { return net::read_with_timeout<int>(receiver, 2s); });
    EXPECT_EQ(value, 42);
}

TEST_F(AsioTest, ReadTimeoutRestoresModeAndAllowsFurtherOperations) {
    using namespace std::chrono_literals;
    expect_timeout([&] { (void)net::read_with_timeout<int>(receiver, 20ms); });
    EXPECT_FALSE(receiver.non_blocking());
    net::write(sender, 42);
    EXPECT_EQ(net::read<int>(receiver), 42);
}

TEST_F(AsioTest, ExistingNonblockingModeIsPreservedOnSuccessAndTimeout) {
    using namespace std::chrono_literals;
    receiver.non_blocking(true);
    expect_timeout([&] { (void)net::read_with_timeout<int>(receiver, 10ms); });
    EXPECT_TRUE(receiver.non_blocking());
    net::write(sender, 7);
    EXPECT_EQ(net::read_with_timeout<int>(receiver, 2s), 7);
    EXPECT_TRUE(receiver.non_blocking());
}

TEST_F(AsioTest, ZeroAndNegativeTimeoutsDoNotConsumeOrWriteData) {
    using namespace std::chrono_literals;
    net::write(sender, 42);
    expect_timeout([&] { (void)net::read_with_timeout<int>(receiver, 0ms); });
    expect_timeout([&] { net::write_with_timeout(receiver, 99, -1ms); });
    EXPECT_EQ(net::read<int>(receiver), 42);
    EXPECT_FALSE(receiver.non_blocking());
}

TEST_F(AsioTest, NonfiniteTimeoutsAreRejected) {
    for (double value : {std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::infinity()}) {
        EXPECT_THROW(net::write_with_timeout(sender, 1, std::chrono::duration<double>(value)), std::invalid_argument);
    }
    EXPECT_FALSE(sender.non_blocking());
}

TEST_F(AsioTest, VeryLargeTimeoutDoesNotOverflowTheDeadline) {
    net::write_with_timeout(sender, 42, std::chrono::hours::max());
    EXPECT_EQ(net::read_with_timeout<int>(receiver, std::chrono::hours::max()), 42);
}

TEST_F(AsioTest, CallbackExceptionRestoresTimeoutState) {
    using namespace std::chrono_literals;

    struct Exploding {
        void write(Tcp::socket &) const {
            throw std::runtime_error("write");
        }

        void read(Tcp::socket &) {
            throw std::runtime_error("read");
        }
    };

    EXPECT_THROW(net::write_with_timeout(sender, Exploding{}, 1s), std::runtime_error);
    EXPECT_THROW((void)net::read_with_timeout<Exploding>(receiver, 1s), std::runtime_error);
    EXPECT_FALSE(sender.non_blocking());
    EXPECT_FALSE(receiver.non_blocking());
    net::write(sender, 7);
    EXPECT_EQ(net::read<int>(receiver), 7);
}

TEST_F(AsioTest, TimedReadPropagatesEofInsteadOfReportingTimeout) {
    using namespace std::chrono_literals;
    sender.shutdown(Tcp::socket::shutdown_send);
    try {
        (void)net::read_with_timeout<int>(receiver, 2s);
        FAIL() << "Expected EOF";
    } catch (boost::system::system_error const & error) {
        EXPECT_EQ(error.code(), boost::asio::error::eof);
    }
    EXPECT_FALSE(receiver.non_blocking());
}

TEST_F(AsioTest, WriteTimeoutWhenPeerDoesNotDrainSocket) {
    using namespace std::chrono_literals;
    sender.set_option(boost::asio::socket_base::send_buffer_size(4096));
    receiver.set_option(boost::asio::socket_base::receive_buffer_size(4096));

    struct SaturatingWriter {
        void write(Tcp::socket & socket) const {
            const std::array<double, 8192> block{};
            for (;;) {
                net::write(socket, block);
            }
        }

        void read(Tcp::socket &) {
        }
    };

    const SaturatingWriter value;
    expect_timeout([&] { net::write_with_timeout(sender, value, 30ms); });
    EXPECT_FALSE(sender.non_blocking());
}

TEST_F(AsioTest, TimeoutIsSharedAcrossAllElements) {
    using namespace std::chrono_literals;
    expect_timeout([&] {
        transfer(
            [&] {
                net::write(sender, std::size_t{2});
                std::this_thread::sleep_for(80ms);
                net::write(sender, 1);
                std::this_thread::sleep_for(80ms);
                net::write(sender, 2);
            },
            [&] { (void)net::read_with_timeout<std::vector<int>>(receiver, 120ms); });
    });
}
