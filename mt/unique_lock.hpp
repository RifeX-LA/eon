#pragma once

#include <tuple>
#include <mutex>

namespace eon::mt {

    template <typename... Mutexes>
    requires (sizeof...(Mutexes) > 0)
    class unique_lock {
    public:
        [[nodiscard]] explicit unique_lock(Mutexes &... mutexes) : m_mutexes(std::addressof(mutexes)...) {
            lock_impl();
        }

        [[nodiscard]] explicit unique_lock(std::adopt_lock_t, Mutexes &... mutexes) noexcept : m_mutexes(std::addressof(mutexes)...), m_owns(true) {}

        explicit unique_lock(std::defer_lock_t, Mutexes &... mutexes) noexcept : m_mutexes(std::addressof(mutexes)...), m_owns(false) {}

        [[nodiscard]] explicit unique_lock(std::try_to_lock_t, Mutexes &... mutexes): m_mutexes(std::addressof(mutexes)...) {
            try_lock_impl();
        }

        [[nodiscard]] unique_lock(unique_lock && other) noexcept : m_mutexes(other.m_mutexes), m_owns(other.m_owns) {
            other.m_owns = false;
        }

        unique_lock & operator=(unique_lock && other) noexcept {
            unique_lock{std::move(other)}.swap(*this);
            return *this;
        }

        ~unique_lock() {
            if (m_owns) {
                unlock_impl();
            }
        }

        void lock() {
            validate_lock();
            lock_impl();
        }

        [[nodiscard]] bool try_lock() {
            validate_lock();
            try_lock_impl();
            return m_owns;
        }

        void unlock() {
            if (!m_owns) {
                throw std::system_error(std::make_error_code(std::errc::operation_not_permitted));
            }

            unlock_impl();
            m_owns = false;
        }

        [[nodiscard]] bool owns_lock() const noexcept {
            return m_owns;
        }

        explicit operator bool() const noexcept {
            return m_owns;
        }

        void swap(unique_lock & other) noexcept {
            std::swap(m_mutexes, other.m_mutexes);
            std::swap(m_owns, other.m_owns);
        }

    private:
        void validate_lock() const {
            if (m_owns) {
                throw std::system_error(std::make_error_code(std::errc::resource_deadlock_would_occur));
            }
        }

        void lock_impl() {
            auto impl = [](Mutexes *... mutexes) {
                if constexpr (sizeof...(mutexes) == 1) {
                    (mutexes->lock(), ...);
                }
                else {
                    std::lock(*mutexes...);
                }
            };

            std::apply(impl, m_mutexes);
            m_owns = true;
        }

        void try_lock_impl() {
            auto impl = [](Mutexes *... mutexes) {
                if constexpr (sizeof...(mutexes) == 1) {
                    return (mutexes->try_lock() && ...) ? -1 : 0;
                }
                else {
                    return std::try_lock(*mutexes...);
                }
            };

            m_owns = (std::apply(impl, m_mutexes) == -1);
        }

        void unlock_impl() {
            std::apply([](Mutexes *... mutexes) { (..., mutexes->unlock()); }, m_mutexes);
        }

    private:
        std::tuple<Mutexes *...> m_mutexes;
        bool m_owns;
    };

}
