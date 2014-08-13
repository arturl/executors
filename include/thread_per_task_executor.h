#ifndef THREAD_EXECUTOR
#define THREAD_EXECUTOR

#include "executor.h"
#include <vector>
#include <thread>

using namespace std;

class thread_per_task_executor {
private:
    vector<thread> thread_vec;
    thread_per_task_executor() {}
public:

    static thread_per_task_executor& get_thread_per_task_executor()
    {
        static thread_per_task_executor instance;
        return instance;
    }

    virtual ~thread_per_task_executor() {
        for (thread& t : thread_vec) {
            if (t.joinable())
                t.join();
        }
    }

    template<class Func>
    void add(Func closure) {
        thread_vec.emplace_back(closure);
    }

    virtual size_t uninitiated_task_count() const {
        // TBD
        return 0;
    }
};

#endif