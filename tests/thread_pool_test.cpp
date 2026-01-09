#include <gtest/gtest.h>
#include "thread_pool/thread_pool.hpp"

#include <atomic>
#include <chrono>
#include <vector>
#include <stdexcept>

TEST(ThreadPool, ConstructDestruct) {
    tp::ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(), 4);
}

TEST(ThreadPool, SingleTask) {
    tp::ThreadPool pool(2);
    auto future = pool.submit([] {return 42;});
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPool, TaskWithArgs) {
    tp::ThreadPool pool(2);
    auto future = pool.submit([](int a, int b) {return a+b;}, 10, 20);
    EXPECT_EQ(future.get(), 30);
}

TEST(ThreadPool, VoidTask) {
    tp::ThreadPool pool(2);
    // we want atomic here because this is technically a race
    // though it will work regardless, santiizers will flag as race.
    std::atomic<bool> executed{false};
    auto future = pool.submit([&executed] {
        executed.store(true);
    });
    future.get();
    EXPECT_TRUE(executed.load());
}

TEST(ThreadPool, MultipleTasksWithResults) {
    tp::ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for(int i=0; i<100; ++i){
        futures.push_back(pool.submit([i] {return i*2;}));
    }
    for(int i=0; i<100; ++i) {
        EXPECT_EQ(futures[i].get(), i*2);
    }
}

TEST(ThreadPool, TasksRunConcurrently) {
    tp::ThreadPool pool(4);
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};

    std::vector<std::future<void>> futures;

    for(int i=0; i<8; ++i){
        futures.push_back(pool.submit([&] {
            // note: increment & decrement on atomic ints are equivalent to fetch_add -> it is atomic by default
            int current = ++concurrent_count;
            int prev_max = max_concurrent.load();         
            /*
                this loop will exit when prev_max >= current (in which case our current counter is irrelevant)
                OR the compare_exchange_weak has succeeded (we have successfully updated the max_concurrent with current)
                this means that we will ONLY update the max_concurrent with current if our prev_max == max_concurrent and the increment will only happen once.

                note: on compare exchange_weak failure, we update prev_max with max_concurrent
                    - meaning we dont need to do prev_max.load() inside the loop
                note: we do compare_exchange_weak since this is in a loop & we're retrying anyway - spurious failure is ok
            */
            while(prev_max < current &&
                !max_concurrent.compare_exchange_weak(prev_max, current)) {}
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            --concurrent_count;
        }));
    }
    for(auto& f : futures) {
        f.get();
    }
    EXPECT_GE(max_concurrent.load(), 2);
}

TEST(ThreadPool, ExceptionPropagation) {
    tp::ThreadPool pool(2);
    auto future = pool.submit([]() -> int {
        throw std::runtime_error("intentional");
    });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPool, PoolWorksAfterException) {
    tp::ThreadPool pool(2);
    auto bad_future = pool.submit([]() -> int {
        throw std::runtime_error("oops");
    });
    EXPECT_THROW(bad_future.get(), std::runtime_error);
    auto good_future = pool.submit([]{return 123;});
    EXPECT_EQ(good_future.get(), 123);
}

TEST(ThreadPool, ShutdownWaitsForTasks) {
    std::atomic<int> completed{0};
    {
        tp::ThreadPool pool(2);
        for(int i=0; i<5; ++i) {
            pool.submit([&completed] {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                ++completed;
            });
        }
        //destructor calls shutdown, should wait
    }
    EXPECT_EQ(completed.load(), 5);
}

TEST(ThreadPool, EnqueueAfterShutdownThrows) {
    tp::ThreadPool pool(2);
    pool.shutdown();
    EXPECT_THROW(pool.submit([]{}), std::runtime_error);
}

TEST(ThreadPool, DoubleShutdownIsSafe) {
    tp::ThreadPool pool(2);
    pool.shutdown();
    pool.shutdown();
}

TEST(ThreadPool, PendingTasksCount) {
    tp::ThreadPool pool(1); //single worker
    std::atomic<bool> block{true};
    pool.submit([&block] {
        while(block.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    // give time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    pool.submit([]{});
    pool.submit([]{});
    EXPECT_EQ(pool.pending_tasks(), 2);
    block.store(false); //unblock
}

TEST(ThreadPool, ZeroThreadsThrowsOrHandles) {
    EXPECT_THROW(tp::ThreadPool pool(0), std::invalid_argument);
}