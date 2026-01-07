#include "thread_pool/thread_pool.hpp"

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace tp {

class ThreadPool::Impl {
public:
    explicit Impl(std::size_t num_threads);
    ~Impl();
    
    void enqueue(std::function<void()> task);
    void shutdown();

    std::size_t thread_count() const noexcept { return workers_.size(); }
    std::size_t pending_tasks() const noexcept;

private:
    void worker_loop();
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;

    std::atomic<bool> stop_{false};
};

// implementation of Impl methods (placeholder for now)
ThreadPool::Impl::Impl(std::size_t num_threads) {
    workers_.reserve(num_threads);
    for(std::size_t i=0; i<num_threads; ++i) {
        workers_.emplace_back([this] {worker_loop();});
    }
}

ThreadPool::Impl::~Impl() {
    shutdown();
}

void ThreadPool::Impl::worker_loop() {
    // phase 4
}

void ThreadPool::Impl::enqueue(std::function<void()> task) {
    //phase 5
}

void ThreadPool::Impl::shutdown() {
    //phase 6
}

std::size_t ThreadPool::Impl::pending_tasks() const noexcept {
    std::lock_guard lock(mutex_);
    return tasks_.size();
}


ThreadPool::ThreadPool(std::size_t num_threads)
    : impl_(std::make_unique<Impl>(num_threads)) {}

ThreadPool::~ThreadPool() = default;

void ThreadPool::shutdown() { impl_->shutdown(); }

std::size_t ThreadPool::thread_count() const noexcept {
    return impl_->thread_count();
}

std::size_t ThreadPool::pending_tasks() const noexcept {
    return impl_->pending_tasks();
}

} // namespace tp