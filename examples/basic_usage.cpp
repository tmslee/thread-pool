#include "thread_pool/thread_pool.hpp"
#include <iostream>

int main() {
    tp::ThreadPool pool(4);
    auto future = pool.submit([](int x) {return x*2;}, 21);
    std::cout<<future.get()<<std::endl; //42;
    return 0;
}