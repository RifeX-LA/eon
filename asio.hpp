#pragma once

#include <concepts>
#include <new>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include <array>
#include <ranges>
#include <optional>
#include <expected>

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <boost/fusion/include/for_each.hpp>

#include <eon/concepts.hpp>
#include <eon/nullable.hpp>

namespace eon::asio {

    namespace detail {

        template <typename T>
        concept endian_reversable = std::integral<T> && !std::same_as<T, bool>;


        template <typename T, typename Period, typename Rep>
        void write_with_timeout(boost::asio::ip::tcp::socket & socket, T const & value, std::chrono::duration<Period, Rep> const & timeout) {
            boost::asio::system_timer timer(socket.get_executor());
            boost::asio::async_write(socket, value);
        }


        template <endian_reversable T>
        void write_integral(boost::asio::ip::tcp::socket & socket, T value) {
            auto const value_be = boost::endian::native_to_big(value);
            boost::asio::write(socket, boost::asio::buffer(std::addressof(value_be), sizeof(T)));
        }

        template <endian_reversable T>
        [[nodiscard]] T read_integral(boost::asio::ip::tcp::socket & socket) {
            T value_be;
            boost::asio::read(socket, boost::asio::buffer(std::addressof(value_be), sizeof(T)));
            return boost::endian::big_to_native(value_be);
        }

    }

    namespace detail {

        template <typename T>
        concept asio_serializer = requires (T serializer, boost::asio::ip::tcp::socket & socket)
        {
            typename T::value_type;
            { serializer.write(socket, std::declval<typename T::value_type const &>()) };
            { serializer.read(socket) } -> std::same_as<typename T::value_type>;
        };


        template <typename T>
        concept has_write_member = requires (T value, boost::asio::ip::tcp::socket & socket) { value.write(socket); };

        template <typename T>
        concept has_read_member = requires (T value, boost::asio::ip::tcp::socket & socket) { value.read(socket); };

        template <typename T>
        concept has_write_adl = requires (T value, boost::asio::ip::tcp::socket & socket) { write(socket, value); };

        template <typename T>
        concept has_read_adl = requires (T value, boost::asio::ip::tcp::socket & socket) { read(socket, value); };


        template <typename T>
        [[nodiscard]] consteval asio_serializer auto serializer_for() noexcept;

        template <typename T>
        struct provide_value_type {
            using value_type = T;
        };

        struct bool_serializer final : provide_value_type<bool> {
            static void write(boost::asio::ip::tcp::socket & socket, bool value) {
                write_integral(socket, static_cast<int>(value));
            }

            [[nodiscard]] static bool read(boost::asio::ip::tcp::socket & socket) {
                return static_cast<bool>(read_integral<int>(socket));
            }
        };

        template <typename T>
        requires (has_write_member<T> && has_read_member<T>)
        struct member_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                value.write(socket);
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                T value{};
                value.read(socket);
                return value;
            }
        };

        template <typename T>
        requires (has_write_adl<T> && has_read_adl<T>)
        struct adl_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                ::write(socket, value);
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                T value{};
                ::read(socket, value);
                return value;
            }
        };

        template <typename T>
        requires (std::is_array_v<T>)
        struct carray_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                boost::asio::write(socket, boost::asio::buffer(value));
            }

            [[nodiscard]] static std::decay_t<T> read(boost::asio::ip::tcp::socket & socket) {
                return nullptr;
            }
        };

        template <std::ranges::forward_range T>
        struct range_serializer final : provide_value_type<T> {
            using rng_value_t = std::ranges::range_value_t<T>;

            constexpr static bool can_process_as_buffer = std::ranges::contiguous_range<T>
                                                       && std::is_fundamental_v<rng_value_t>
                                                       && !std::integral<rng_value_t>
                                                       && !std::same_as<rng_value_t, bool>;

            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                auto const size = static_cast<std::size_t>(std::ranges::distance(value));
                write_integral(socket, size);

                if constexpr (can_process_as_buffer) {
                    boost::asio::write(socket, boost::asio::buffer(std::ranges::cdata(value), size));
                }
                else {
                    static constexpr auto serializer = serializer_for<rng_value_t>();
                    for (auto && elem : value) {
                        serializer.write(socket, elem);
                    }
                }
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                auto const size = read_integral<std::size_t>(socket);

                if constexpr (can_process_as_buffer) {
                    T rng;
                    if constexpr (requires { rng.resize(size); }) {
                        rng.resize(size);
                    }
                    boost::asio::read(socket, boost::asio::buffer(std::ranges::data(rng), size));

                    return rng;
                }

                static constexpr auto serializer = serializer_for<rng_value_t>();

                return std::views::iota(std::size_t{0}, size)
                     | std::views::transform([&](std::size_t){ return serializer.read(socket); })
                     | std::ranges::to<T>();
            }
        };

        template <tuple_like T>
        struct tuple_like_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                boost::fusion::for_each(value, [&]<typename U>(U && arg) {
                    serializer_for<std::remove_cvref_t<U>>().write(socket, arg);
                });
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                auto read_tuple_impl = [&]<std::size_t... I>(std::index_sequence<I...>) {
                    return T(serializer_for<std::tuple_element_t<I, T>>().read(socket)...);
                };
                return read_tuple_impl(std::make_index_sequence<std::tuple_size_v<T>>());
            }
        };

        template <specialization_of<std::variant> T>
        struct variant_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                write_integral(socket, value.index());
                std::visit([&]<typename U>(U && arg) {
                    serializer_for<std::remove_cvref_t<U>>().write(socket, arg);
                }, value);
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                auto const index = read_integral<std::size_t>(socket);
                if (index == std::variant_npos) {
                    throw std::bad_variant_access();
                }

                std::optional<T> variant_opt;

                auto read_arg_by_index = [&]<std::size_t I> {
                    if (I == index) {
                        static constexpr auto serializer = serializer_for<std::variant_alternative_t<I, T>>();
                        variant_opt.emplace(serializer.read(socket));
                    }
                };
                auto read_variant = [&]<std::size_t... I>(std::index_sequence<I...>) {
                    (read_arg_by_index.template operator()<I>(), ...);
                };
                read_variant(std::make_index_sequence<std::variant_size_v<T>>{});

                return *variant_opt;
            }
        };

        template <specialization_of<std::expected> T>
        struct expected_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                bool const has_value = value.has_value();
                bool_serializer::write(socket, has_value);
                if (has_value) {
                    serializer_for<typename T::value_type>().write(socket, *value);
                }
                else {
                    serializer_for<typename T::error_type>()(socket, value.error());
                }
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                if ([[maybe_unused]] auto const has_value = bool_serializer::read(socket)) {
                    return serializer_for<typename T::value_type>().read(socket);
                }
                return std::unexpected(serializer_for<typename T::error_type>().read(socket));
            }
        };

        template <nullable T>
        struct nullable_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                auto const has_value = static_cast<bool>(value);
                bool_serializer::write(socket, has_value);
                if (has_value) {
                    serializer_for<nullable_value_t<T>>().write(socket, *value);
                }
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                if ([[maybe_unused]] auto const has_value = bool_serializer::read(socket)) {
                    return T{serializer_for<nullable_value_t<T>>().read(socket)};
                }
                return T{};
            }
        };

        template <typename T>
        requires (std::is_enum_v<T>)
        struct enum_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                write_integral(socket, std::to_underlying(value));
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                return static_cast<T>(read_integral<std::underlying_type_t<T>>(socket));
            }
        };

        template <std::integral T>
        struct integral_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T value) {
                write_integral(socket, value);
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                return read_integral<T>(socket);
            }
        };

        template <typename T>
        struct default_serializer final : provide_value_type<T> {
            static void write(boost::asio::ip::tcp::socket & socket, T const & value) {
                boost::asio::write(socket, boost::asio::buffer(std::addressof(value), sizeof(T)));
            }

            [[nodiscard]] static T read(boost::asio::ip::tcp::socket & socket) {
                alignas(T) std::byte buffer[sizeof(T)];
                boost::asio::read(socket, boost::asio::buffer(buffer, sizeof(T)));
                return *std::launder(static_cast<T*>(std::memmove(buffer, buffer, sizeof(T))));

                // if constexpr (std::default_initializable<T>) {
                //     T value{};
                //     default_serializer::read(socket, value);
                //     return value;
                // }
                // else {
                //     alignas(T) std::byte buffer[sizeof(T)];
                //     boost::asio::read(socket, boost::asio::buffer(buffer, sizeof(T)));
                //     return *std::launder(static_cast<T*>(std::memmove(buffer, buffer, sizeof(T))));
                // }
            }

            static void read(boost::asio::ip::tcp::socket & socket, T & value) {
                boost::asio::read(socket, boost::asio::buffer(std::addressof(value), sizeof(T)));
            }
        };

        template <typename T>
        [[nodiscard]] consteval asio_serializer auto serializer_for() noexcept {
            if constexpr (has_write_member<T> && has_read_member<T>) {
                return member_serializer<T>{};
            }
            else if constexpr (has_write_adl<T> && has_read_adl<T>) {
                return adl_serializer<T>{};
            }
            else if constexpr (std::is_array_v<T>) {
                return carray_serializer<T>{};
            }
            else if constexpr (std::ranges::range<T>) {
                return range_serializer<T>{};
            }
            else if constexpr (tuple_like<T>) {
                return tuple_like_serializer<T>{};
            }
            else if constexpr (specialization_of<T, std::variant>) {
                return variant_serializer<T>{};
            }
            else if constexpr (specialization_of<T, std::expected>) {
                return expected_serializer<T>{};
            }
            else if constexpr (nullable<T>) {
                return nullable_serializer<T>{};
            }
            else if constexpr (std::is_enum_v<T>) {
                return enum_serializer<T>{};
            }
            else if constexpr (std::same_as<std::remove_cv_t<T>, bool>) {
                return bool_serializer{};
            }
            else if constexpr (std::integral<T>) {
                return integral_serializer<T>{};
            }
            else {
                return default_serializer<T>{};
            }
        }

    }

    template <typename T>
    void write_as_buffer(boost::asio::ip::tcp::socket & socket, T const & value) {
        detail::default_serializer<T>::write(socket, value);
    }

    template <typename T>
    [[nodiscard]] T read_as_buffer(boost::asio::ip::tcp::socket & socket) {
        return detail::default_serializer<T>::read(socket);
    }

    template <typename T>
    void read_as_buffer(boost::asio::ip::tcp::socket & socket, T & value) {
        detail::default_serializer<T>::read(socket, value);
    }

    template <typename T>
    void write(boost::asio::ip::tcp::socket & socket, T const & value) {
        detail::serializer_for<T>().write(socket, value);
    }

    template <typename T>
    [[nodiscard]] T read(boost::asio::ip::tcp::socket & socket) {
        return detail::serializer_for<T>().read(socket);
    }


    namespace detail {
        inline constexpr std::size_t file_chunk_size = 8192;
    }

    inline void write_file(boost::asio::ip::tcp::socket & socket, std::filesystem::path const & src) {
        std::ifstream fin(src.c_str(), std::ios::in | std::ios::binary);
        std::array<char, detail::file_chunk_size> chunk;
        while (fin) {
            fin.read(chunk.data(), chunk.size());
            if (std::streamsize const read_bytes = fin.gcount(); read_bytes > 0) {
                boost::asio::write(socket, boost::asio::buffer(chunk.data(), read_bytes));
            }
        }
    }

    inline void read_file(boost::asio::ip::tcp::socket & socket, std::filesystem::path const & dest) {
        std::ofstream fout(dest.c_str(), std::ios::out | std::ios::binary);
        std::array<char, detail::file_chunk_size> chunk;
        boost::system::error_code ec;
        while (true) {
            auto const read_bytes = static_cast<std::streamsize>(socket.read_some(boost::asio::buffer(chunk), ec));
            if (ec == boost::asio::error::eof) {
                if (read_bytes > 0) {
                    fout.write(chunk.data(), read_bytes);
                }
                break;
            }

            if (ec) {
                throw boost::system::system_error(ec);
            }

            if (read_bytes == 0) {
                break;
            }

            fout.write(chunk.data(), read_bytes);
        }
    }

}
