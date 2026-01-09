#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>

namespace tp {
    
class ThreadPool {
private:
    class Impl;
    std::unique_ptr<Impl> impl_;

public:
    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&) = delete;

    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<typename F, typename... Args>
    auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [func = std::forward<F>(f), ..capturedArgs = std::forward<args>(args)]() mutable {
                return func(std::move(capturedArgs)...);
            }
        )
        std::future<ReturnType> future = task->get_future();
        impl_->enqueue([task = std::move(task)]() {
            (*task)();
        });
        return future;
    };

    [[nodiscard]] std::size_t thread_count() const noexcept;
    [[nodiscard]] std::size_t pending_tasks() const noexcept;

    void shutdown();

};

} //namespace tp

#endif