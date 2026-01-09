//include guards
#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>

namespace tp { //namespace to avoid collision
    
class ThreadPool {
private:
    //PIMPL forward declaration of implementation details for optimized compilation & encapsulation 
    class Impl;
    std::unique_ptr<Impl> impl_;
    void enqueue(std::function<void()> task);

public:
    //explicit constructor for clarity & prevent accidental conversion
    explicit ThreadPool(std::size_t num_threads); 
    ~ThreadPool();

    //deleting copy constructors - thread pool manages threads, and those threads hold pointers to our class's internal objects (i.e. queue). copying a thread pool doesnt make intuitive sense - does it duplicate the threads? does it use the same queue? is it a new queue?
    //we choose to not implement copy constructor for this reason.
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&) = delete;

    //deleting move constructors - if you move a thread pool, the threads will be referencing impl_ from the old thread pool and will be referencing the old address -> dangling ptr -> UB.
    //in theory this can be made moveable with extra indirection but forgo this in our implementation
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<typename F, typename... Args>
    //async retrieval of results with std::future + perfect forwarding with submit function signature
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;

    // nodiscard for getters
    [[nodiscard]] std::size_t thread_count() const noexcept;
    [[nodiscard]] std::size_t pending_tasks() const noexcept;

    void shutdown();

};

template<typename F, typename... Args>
auto ThreadPool::submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> 
{
    /*
        note on std::packaged_task and std::function
        std::packaged_task:
        - catches exceptions and stores them in future. this makes our thread pool exception safe
        - move only bc it owns a std::promise internally which represents a unique comms channel to a future
            - 1 producer : 1 consumer. copying will break this 1 writer contract
        
        std::function:
        - type erased, can hold and callable
            - this is achieved by storing callable on heap & manages through internal base class ptr. this interface allows copying.
        - to support this whatever you put inside MUST be copyable
    */
   
    using ReturnType = std::invoke_result_t<F, Args...>;

    // std::packaged_task is move only but std::function needs to be copyable -> we wrap in shared_ptr
    // std::make_shared<std::packaged_task<R>>
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        [func = std::forward<F>(f), ...capturedArgs = std::forward<Args>(args)]() mutable {
            //need mutable here since std::move(capturedArgs)... modifies them & captures are const by default
            return func(std::move(capturedArgs)...);
        }
    );
    std::future<ReturnType> future = task->get_future();
    enqueue([task = std::move(task)]() {
        (*task)();
    });
    return future;
}

} //namespace tp

#endif