#pragma once

#include <iosfwd>
#include <print>

namespace eon {

    class lifetime final {
    public:
        lifetime() : m_id(0) {
            on_create();
            std::println("lifetime() [default constructor]");
        }

        explicit lifetime(int i) : m_id(i) {
            on_create();
            std::println("lifetime(int) [int constructor]");
        }

        lifetime(lifetime const & other) : m_id(other.m_id) {
            on_create();
            std::println("lifetime(lifetime const &) [copy constructor]");
        }

        lifetime(lifetime && other) noexcept : m_id(other.m_id) {
            on_create();
            std::println("lifetime(lifetime &&) [move constructor]");
        }

        lifetime & operator=(lifetime const & rhs) {
            std::println("lifetime& operator=(lifetime const &) [copy assignment] {} = {}", m_id, rhs.m_id);
            m_id = rhs.m_id;
            return *this;
        }

        lifetime & operator=(lifetime && rhs) noexcept {
            std::println("lifetime& operator=(lifetime &&) [move assignment] {} = {}", m_id, rhs.m_id);
            m_id = rhs.m_id;
            return *this;
        }

        ~lifetime() noexcept {
            --s_alive;
            std::println("~lifetime({}) [destructor]", m_id);
        }

        [[nodiscard]] int id() const noexcept {
            return m_id;
        }

        auto operator<=>(lifetime rhs) const {
            std::println("lifetime::operator <=> [three-way comparison operator] {} <=> {}", m_id, rhs.m_id);
            return m_id <=> rhs.m_id;
        }

        bool operator==(lifetime rhs) const {
            std::println("lifetime::operator == [equality operator] {} == {}", m_id, rhs.m_id);
            return m_id == rhs.m_id;
        }

        bool operator!=(lifetime rhs) const {
            std::println("lifetime::operator != [inequality operator] {} != {}", m_id, rhs.m_id);
            return m_id != rhs.m_id;
        }

        bool operator<(lifetime rhs) const {
            std::println("lifetime::operator < [less operator] {} < {}", m_id, rhs.m_id);
            return m_id < rhs.m_id;
        }

        bool operator<=(lifetime rhs) const {
            std::println("lifetime::operator <= [less equality operator] {} < {}", m_id, rhs.m_id);
            return m_id <= rhs.m_id;
        }

        bool operator>(lifetime rhs) const {
            std::println("lifetime::operator > [greater operator] {} > {}", m_id, rhs.m_id);
            return m_id > rhs.m_id;
        }

        bool operator>=(lifetime rhs) const {
            std::println("lifetime::operator >= [greater equality operator] {} >= {}", m_id, rhs.m_id);
            return m_id >= rhs.m_id;
        }

        template <typename CharT, typename Traits>
        friend std::basic_ostream<CharT, Traits> & operator<<(std::basic_ostream<CharT, Traits> & os, lifetime l) {
            return os << std::format("lifetime({})", l.m_id);
        }

        [[nodiscard]] static std::size_t total() noexcept {
            return s_total;
        }

        [[nodiscard]] static std::size_t alive() noexcept {
            return s_alive;
        }

    private:
        int m_id;

        static std::size_t s_total;
        static std::size_t s_alive;

        static void on_create() {
            ++s_total;
            ++s_alive;
        }
    };

    std::size_t lifetime::s_total = 0;
    std::size_t lifetime::s_alive = 0;

} // namespace eon

template <>
struct std::formatter<eon::lifetime> : std::formatter<int> {
    [[nodiscard]] auto format(eon::lifetime l, std::format_context & ctx) const {
        std::format_to(ctx.out(), "lifetime(");
        std::formatter<int>::format(l.id(), ctx);
        return std::format_to(ctx.out(), ")");
    }
};
