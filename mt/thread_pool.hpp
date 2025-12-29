#pragma once

#include <thread>
#include <future>
#include <condition_variable>
#include <vector>
#include <queue>
#include <set>
#include <functional>

#include <eon/concepts.hpp>
#include <eon/mt/concurrency_info.hpp>


namespace eon::mt {

    template <typename R = void>
    class thread_pool {
        using task_t = std::packaged_task<R()>;

    public:
        explicit thread_pool(unsigned threads_count = concurrent_available()) {
            add_threads(threads_count);
        }

        template <typename Fn>
        requires (std::is_invocable_r_v<R, std::decay_t<Fn>>)
        [[nodiscard]] std::future<R> add_task(Fn && fn) {
            task_t task(std::forward<Fn>(fn));
            auto future = task.get_future();
            {
                std::scoped_lock lock(m_queue_mutex);
                m_tasks.push_back(std::move(task));
            }
            m_cv.notify_one();

            return future;
        }

        void resize(unsigned threads_count) {
            if (threads_count <= m_threads.size()) {
                m_threads.resize(threads_count);
            }
            else {
                add_threads(threads_count - m_threads.size());
            }
        }

        void force_resize(unsigned threads_count) {
            if (threads_count < m_threads.size()) {
                std::size_t const threads_count_to_stop = m_threads.size() - threads_count;
                auto ids_to_stop = std::span(m_threads).last(threads_count_to_stop) | std::views::transform(&std::jthread::get_id);
                {
                    std::scoped_lock lock(m_force_stop_mutex);
                    m_threads_to_force_stop.insert_range(ids_to_stop);
                }
                m_threads.resize(threads_count);
                m_cv.notify_all();
            }
            else {
                add_threads(threads_count - m_threads.size());
            }
        }

        void request_stop() {
            std::ranges::for_each(m_threads, &std::jthread::request_stop);
        }

        void wait() {
            request_stop();
            std::ranges::for_each(m_threads, &std::jthread::join);
        }

        void wait_and_deallocate() {
            m_threads.clear();
            m_threads.shrink_to_fit();
            m_threads_to_force_stop.clear();
            m_tasks.clear();
            m_tasks.shrink_to_fit();
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return m_threads.size();
        }

    private:
        void add_threads(unsigned threads_count) {
            m_threads.reserve(m_threads.size() + threads_count);
            for (unsigned i = 0; i < threads_count; ++i) {
                m_threads.emplace_back(std::bind_front(&thread_pool::work, this));
            }
        }

        void work(std::stop_token stop_token) {
            task_t task;
            std::set<std::jthread::id>::iterator it;

            while (true) {
                std::unique_lock lock(m_queue_mutex);
                bool const has_tasks = m_cv.wait(lock, stop_token, [this] { return !m_tasks.empty(); });

                {
                    std::scoped_lock force_stop_lock(m_force_stop_mutex);
                    it = m_threads_to_force_stop.find(std::this_thread::get_id());
                    if (it != m_threads_to_force_stop.end()) {
                        break;
                    }
                }

                if (!has_tasks) {
                    return;
                }

                task = std::move(m_tasks.front());
                m_tasks.pop_front();
                lock.unlock();

                task();
            }

            std::scoped_lock lock(m_force_stop_mutex);
            m_threads_to_force_stop.erase(it);
        }

    private:
        std::mutex m_queue_mutex;
        std::mutex m_force_stop_mutex;
        std::condition_variable_any m_cv;

        std::deque<task_t> m_tasks;
        std::set<std::jthread::id> m_threads_to_force_stop;
        std::vector<std::jthread> m_threads;
    };

}