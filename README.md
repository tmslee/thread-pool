# Thread Pool
a simple modern c++20 thread pool implementation

## Features
- Fixed number of worker threads
- Task submission with `std::future` results
- perfect forwarding of task arguments
- Exception propagation to caller
- clean shutdown (waits for pending tasks)
- thread-safe

## Requirements
- C++20 compiler (GCC 10+, Clang 11+)
- CMake 3.20+

## Building
```bash
cmake -B build
cmake --build build
```
## Building with sanitizers
```bash
cmake -B build -DENABLE_SANITIZERS=ON
cmake --build build
```

## Running Tets
```bash
ctest --test-dir build --output-on-failure
```

## Usage
```cpp
#include <thread_pool/thread_pool.hpp>
#include

int main() {
    tp::ThreadPool pool(4);
    // submit task with return value
    auto future = pool.submit([](int x) {return x*2;}, 21);
    std::cout<<future.get()<<std::endl; //42

    // submit void task
    pool.submit([]{std::cout<<"hello from thread pool\n";}).get();

    //submit multiple tasks
    std::vector<std::future> futures;
    for(int i=0; i<10; ++i) {
        futures.push_back(pool.submit([i]{return i*i}));
    }

    for (auto& f : futures) {
        std::cout<<f.get<<" ";
    }
    std::cout<<std::endl;
    // destructor waits for all tasks to complete
}
```

## API
### `ThreadPool(std::size_t num_threads)`
construct pool with specified number of workers. throws `std::invalid_argument` if `num_threads` is 0
### `submit(F&& f, Args&&... args) -> std::future<R>`
submit a task, returns future for the results. throws `std::runtime_error` if pool is stopped
### `shutdown()`
stop accepting tasks, wait for pending tasks to complete, join workers. called automatically by destructor, safe to call multiple times
### `thread_count() const noexcept -> std::size_t`
number of worker threads
### `pending_tasks() const noexcept -> std::size_t`
number of tasks waiting in queue

## Design Notes

### Architecture
The implementation uses the PIMPL (Pointer to Implementation) idiom. The public `ThreadPool` class holds only a `std::unique_ptr<Impl>` to the private implementation. This provides:
- **Compilation firewall**: Changes to internals don't recompile users of the header
- **ABI stability**: `sizeof(ThreadPool)` is always the size of a pointer
- **Encapsulation**: Implementation details (mutex, condition variable, queue) stay hidden

### Why Non-Copyable
The thread pool manages worker threads that hold pointers to internal state (the task queue, mutex, condition variable). Copying raises unanswerable questions:
- Does it duplicate the threads?
- Do both copies share the same queue?
- Is it a new queue with the same tasks?
No intuitive answer exists, so copy construction and assignment are deleted.

### Why Non-Moveable
Worker threads capture `this` pointer to access `impl_`:
```cpp
workers_.emplace_back([this] { worker_loop(); });
```
Moving the `ThreadPool` would relocate `impl_`, but running threads still reference the old address. This creates dangling pointers and undefined behavior. While solvable with extra indirection, we opted for simplicity and deleted move operations.

### Task Submission and Type Erasure
The `submit()` function uses two wrappers to handle arbitrary callables:
**`std::packaged_task`**: Wraps the user's callable and provides exception safety. If a task throws, the exception is captured and rethrown when `future.get()` is called. It's move-only because it owns a `std::promise` internally — the one-producer-one-consumer contract cannot be copied.
**`std::function`**: Used for the task queue. It's type-erased and can hold any callable, but requires copyability.
Since `std::packaged_task` is move-only and `std::function` requires copyable, we wrap the packaged task in `std::shared_ptr`. This makes it copyable while preserving single ownership semantics.

### Perfect Forwarding in Submit
The submit signature uses forwarding references:
```cpp
template<typename F, typename... Args>
auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>>;
```
Arguments are captured into the lambda via `std::forward`, preserving value category:
- lvalues are copied
- rvalues are moved
The lambda is marked `mutable` because `std::move(capturedArgs)...` modifies the captures, and lambda captures are const by default.

### Synchronization
**Mutex + Condition Variable**: The standard producer-consumer pattern. Workers sleep on the condition variable until tasks arrive or shutdown is signaled.
**`std::atomic<bool>` for stop flag**: Technically unnecessary since all accesses are under the mutex. However, it provides:
- Clarity: Signals "this is shared state" to readers
- Future-proofing: Safe if we later add lock-free reads
- Zero cost: `memory_order_relaxed` compiles to regular loads/stores on x86
**`memory_order_relaxed`**: Used because the mutex already provides synchronization. We only need atomicity, not ordering guarantees.

### Worker Loop
Workers wait on a predicate that wakes them for either condition:
```cpp
cv_.wait(lock, [this] {
    return stop_.load(std::memory_order_relaxed) || !tasks_.empty();
});
```
After waking, a separate check determines whether to exit:
```cpp
if (stop_.load(std::memory_order_relaxed) && tasks_.empty()) {
    return;
}
```
This ensures graceful shutdown — workers finish pending tasks before exiting.

### Mutable Mutex
The mutex is `mutable` to allow locking in `const` methods like `pending_tasks()`. Without it, `const` methods couldn't acquire the lock.

## Exception handling
exceptions thrown by tasks are captured and rethrown when calling `future.get()`:
```cpp
auto future = pool.submit([]{throw std::runtime_error("oops");});
try {
    future.get();
} catch (const std::runtime_error& e) {
    std::cout<<e.what()<<std::endl; //"oops"
}
```
the pool continues working after a task throws

## License
MIT
