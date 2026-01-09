#include "thread_pool/thread_pool.hpp"
#include <iostream>

int main() {
    tp::ThreadPool pool(4);

    std::cout<<"Thread pool with " << pool.thread_count() << " workers\n";

    //single task with args
    auto future = pool.submit([](int x, int y){return x+y;}, 10, 32);
    std::cout<<"10+32 = "<<future.get()<<std::endl;

    //multiple parallel tasks
    std::vector<std::future<int>> futures;
    for(int i=0; i<10; ++i) {
        futures.push_back(pool.submit([i] {
            return i*i;
        }));
    }
    std::cout<<"Squares: ";
    for(auto& f: futures) {
        std::cout<<f.get()<<" ";
    }
    std::cout<<std::endl;
    
    std::cout<<"Pending tasks: "<<pool.pending_tasks()<<std::endl;
    return 0;
}