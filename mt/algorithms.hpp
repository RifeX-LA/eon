#pragma once

#include <vector>
#include <ranges>
#include <algorithm>
#include <execution>
#include <future>

#include <eon/mt/concurrency_info.hpp>

namespace eon::mt {

    namespace detail {

        [[nodiscard]] inline std::vector<std::size_t> get_chunks_sizes(std::size_t size, std::size_t threads_count) {
            std::size_t const min_chunk_size = size / threads_count;
            std::vector chunks_sizes(threads_count, min_chunk_size);
            std::ranges::fill_n(chunks_sizes.begin(), size % threads_count, min_chunk_size + 1);

            return chunks_sizes;
        }

    }

    template <typename Fn, typename It>
    concept jthread_iter_invocable = std::input_or_output_iterator<It> && (std::invocable<Fn, It, It> || std::invocable<Fn, std::stop_token, It, It>);

    template <typename Fn, typename Rng>
    concept jthread_range_invocable = std::ranges::range<Rng> && (std::invocable<Fn, Rng> || std::invocable<Fn, std::stop_token, Rng>);

    template <typename Fn, typename It>
    concept jthread_invocable = jthread_iter_invocable<Fn, It> || jthread_range_invocable<Fn, std::ranges::subrange<It>>;


    template <std::ranges::forward_range Rng, jthread_invocable<std::ranges::iterator_t<Rng>> Fn>
    void for_each_chunk(Rng && rng, Fn fn) {
        if (std::ranges::empty(rng)) {
            return;
        }

        std::size_t const size = std::ranges::distance(rng);
        std::size_t threads_count = concurrent_optimal(size);
        if (threads_count < size) {
            ++threads_count; // +1 for current thread
        }

        std::vector<std::jthread> threads;
        threads.reserve(threads_count);

        auto it = std::ranges::begin(rng);
        for (std::size_t const chunk_size : detail::get_chunks_sizes(size, threads_count) | std::views::take(threads_count - 1)) {
            auto chunk_end = std::ranges::next(it, chunk_size);
            if constexpr (jthread_iter_invocable<Fn, std::ranges::iterator_t<Rng>>) {
                threads.emplace_back(fn, std::move(it), chunk_end);
            }
            else {
                threads.emplace_back(fn, std::ranges::subrange(std::move(it), chunk_end));
            }
            it = std::move(chunk_end);
        }

        auto end = std::ranges::next(it, std::ranges::end(rng));
        if constexpr (jthread_iter_invocable<Fn, std::ranges::iterator_t<Rng>>) {
            std::invoke(fn, std::move(it), std::move(end));
        }
        else {
            std::invoke(fn, std::ranges::subrange(std::move(it), std::move(end)));
        }
    }

    template <std::forward_iterator It, std::sentinel_for<It> Sent, jthread_invocable<It> Fn>
    void for_each_chunk(It it, Sent sent, Fn fn) {
        for_each_chunk(std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn));
    }


    template <std::ranges::forward_range Rng, std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Fn>
    void for_each(Rng && rng, Fn fn) {
        for_each_chunk(std::forward<Rng>(rng), [&](auto beg, auto end) {
            std::ranges::for_each(std::move(beg), std::move(end), fn);
        });
    }

    template <std::forward_iterator It, std::sentinel_for<It> Sent, std::indirectly_unary_invocable<It> Fn>
    void for_each(It it, Sent sent, Fn fn) {
        mt::for_each(std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn));
    }

    template <typename ExecPolicy, std::ranges::forward_range Rng, std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Fn>
    requires (std::is_execution_policy_v<std::remove_cvref_t<ExecPolicy>>)
    void for_each(ExecPolicy && exec, Rng && rng, Fn fn) {
        auto common_rng = rng | std::views::common;
        std::for_each(exec, std::ranges::begin(common_rng), std::ranges::end(common_rng), std::move(fn));
    }

    template <typename ExecPolicy, std::forward_iterator It, std::sentinel_for<It> Sent, std::indirectly_unary_invocable<It> Fn>
    requires (std::is_execution_policy_v<std::remove_cvref_t<ExecPolicy>>)
    void for_each(ExecPolicy && exec, It it, Sent sent, Fn fn) {
        mt::for_each(exec, std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn));
    }


    template <std::ranges::forward_range Rng, jthread_invocable<std::ranges::iterator_t<Rng>> Fn>
    [[nodiscard]] auto for_each_chunk_async(Rng && rng, Fn fn, std::launch policy = std::launch::async | std::launch::deferred) {
        using iter_t = std::ranges::iterator_t<Rng>;
        using result_t = std::conditional_t<jthread_iter_invocable<Fn, iter_t>,
                                            std::invoke_result_t<Fn, iter_t, iter_t>,
                                            std::invoke_result_t<Fn, std::ranges::subrange<iter_t>>>;

        using futures_container_t = std::vector<std::future<result_t>>;

        if (std::ranges::empty(rng)) {
            return futures_container_t{};
        }

        std::size_t const size = std::ranges::distance(rng);
        std::size_t const threads_count = concurrent_optimal(size);

        futures_container_t futures;
        futures.reserve(threads_count);

        auto it = std::ranges::begin(rng);
        for (std::size_t const chunk_size : detail::get_chunks_sizes(size, threads_count)) {
            auto chunk_end = std::ranges::next(it, chunk_size);
            if constexpr (jthread_iter_invocable<Fn, iter_t>) {
                futures.emplace_back(std::async(policy, fn, std::move(it), chunk_end));
            }
            else {
                futures.emplace_back(std::async(policy, fn, std::ranges::subrange(std::move(it), chunk_end)));
            }
            it = std::move(chunk_end);
        }

        return futures;
    }

    template <std::forward_iterator It, std::sentinel_for<It> Sent, jthread_invocable<It> Fn>
    [[nodiscard]] auto for_each_chunk_async(It it, Sent sent, Fn fn, std::launch policy = std::launch::async | std::launch::deferred) {
        return for_each_chunk_async(std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn), policy);
    }


    template <std::ranges::forward_range Rng, std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Fn>
    [[nodiscard]] auto for_each_async(Rng && rng, Fn fn, std::launch policy = std::launch::async | std::launch::deferred) {
        using fn_result_t = std::indirect_result_t<Fn, std::ranges::iterator_t<Rng>>;

        std::vector<std::future<fn_result_t>> futures;
        if constexpr (std::ranges::sized_range<Rng>) {
            futures.reserve(std::ranges::size(rng));
        }

        std::ranges::for_each(std::forward<Rng>(rng), [&]<typename T>(T && value) {
            if constexpr (std::ranges::borrowed_range<Rng> && !std::is_rvalue_reference_v<T &&>) {
                futures.emplace_back(std::async(policy, fn, std::ref(value)));
            }
            else {
                futures.emplace_back(std::async(policy, fn, std::forward<T>(value)));
            }
        });

        return futures;
    }

    template <std::forward_iterator It, std::sentinel_for<It> Sent, std::indirectly_unary_invocable<It> Fn>
    [[nodiscard]] auto for_each_async(It it, Sent sent, Fn fn, std::launch policy = std::launch::async | std::launch::deferred) {
        return for_each_async(std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn), policy);
    }
}
