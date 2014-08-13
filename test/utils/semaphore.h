#pragma once

#include <condition_variable>
#include <mutex>

namespace utils
{
class semaphore
{
private:
    std::mutex mutex_;
    std::condition_variable condition_;
    unsigned long count_;
public:
    semaphore(unsigned long count) : count_(count)  {}
    ~semaphore() {}

    void notify()
    {
        std::lock_guard<std::mutex> lk(mutex_);
        --count_;
        condition_.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lk(mutex_);
        condition_.wait(lk, [this]{ return count_ == 0; });
    }
};
}