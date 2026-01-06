#include <gtest/gtest.h>
#include "thread_pool/thread_pool.hpp"

TEST(ThreadPool, Constructs) {
    tp::ThreadPool pool(4);
    EXPECT_EQ(pool.thread_count(), 4);
}