#pragma once

#include <vector>
#include <ranges>
#include <algorithm>
#include <execution>

#include <eon/mt/concurrency_info.hpp>

namespace eon::mt {

    template <typename Fn, typename It>
    concept jthread_iter_invocable = std::invocable<Fn, It, It> || std::invocable<Fn, std::stop_token, It, It>;

    template <typename Fn, typename Rng>
    concept jthread_range_invocable = std::invocable<Fn, Rng> || std::invocable<Fn, std::stop_token, Rng>;

    template <typename Fn, typename It>
    concept jthread_invocable = jthread_iter_invocable<Fn, It> || jthread_range_invocable<Fn, std::ranges::subrange<It>>;


    template <std::ranges::forward_range Rng, jthread_invocable<std::ranges::iterator_t<Rng>> Fn>
    void for_each_chunk(Rng && rng, Fn fn) {
        if (std::ranges::empty(rng)) {
            return;
        }

        std::size_t const size = std::ranges::distance(rng);
        std::size_t const threads_count = concurrent_optimal(size);
        std::size_t const min_chunk_size = size / threads_count;

        std::vector chunk_sizes(threads_count, min_chunk_size);
        std::ranges::fill_n(chunk_sizes.begin(), size % threads_count, min_chunk_size + 1);

        std::vector<std::jthread> threads;
        threads.reserve(threads_count);

        auto it = std::ranges::begin(rng);
        for (std::size_t const chunk_size : chunk_sizes) {
            auto chunk_end = std::ranges::next(it, chunk_size);
            if constexpr (jthread_iter_invocable<Fn, std::ranges::iterator_t<Rng>>) {
                threads.emplace_back(fn, std::move(it), chunk_end);
            }
            else {
                threads.emplace_back(fn, std::ranges::subrange(std::move(it), chunk_end));
            }
            it = std::move(chunk_end);
        }
    }

    template <std::forward_iterator It, std::sentinel_for<It> Sent, jthread_invocable<It> Fn>
    void for_each_chunk(It it, Sent sent, Fn fn) {
        for_each_chunk(std::ranges::subrange(std::move(it), std::move(sent)), std::move(fn));
    }


    template <std::ranges::forward_range Rng, std::indirectly_unary_invocable<std::ranges::iterator_t<Rng>> Fn>
    void for_each(Rng && rng, Fn fn) {
        for_each_chunk(std::forward<Rng>(rng), [fn](auto beg, auto end) {
            std::ranges::for_each(std::move(beg), std::move(end), std::move(fn));
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

}
