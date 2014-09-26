#pragma once

#include <platform_thread_pool.h>

using namespace std;

class serial_executor {
    thread_pool serial_queue;    
private:
    abstract_executor_ref m_executor;

public:
    explicit serial_executor(abstract_executor_ref underlying_executor) :
        serial_queue(1),
        m_executor(underlying_executor) {}

    abstract_executor_ref underlying_executor() {
        return m_executor;
    }

    virtual ~serial_executor() {
    }

    void add(function<void()> closure) {
        serial_queue.add([=] {
            std::condition_variable wake;
            std::atomic<bool> done;
            m_executor.add([&]() {
                closure();
                done = true;
                wake.notify_one();
            });
            std::mutex lock;
            std::unique_lock<std::mutex> guard(lock);
            wake.wait(guard, [&](){return !!done;});
        });
    }
};