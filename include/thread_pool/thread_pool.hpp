#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <cstddef>

namespace tp {
    
class ThreadPool
{
private:
    /* data */
public:
    explicit ThreadPool(std::size_t num_threads);
    ~ThreadPool();
};


} //namespace tp

#endif