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