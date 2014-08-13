#ifndef SYSTEM_EXECUTOR
#define SYSTEM_EXECUTOR

#include "executor.h"
#include "thread_helper.h"

using namespace std;

class system_executor {
private:
    shared_ptr<details::functional_timer_pool> pool;

    // Private default constructor: users must access system_executor by calling get_system_executor
    system_executor() : pool(new details::functional_timer_pool()) {}
public:

    static system_executor& get_system_executor() {
        static system_executor instance;
        return instance;
    }

    template<class Func>
    void add(Func&& closure) {
        pool->submit(std::move(closure));
    }

    template<class Func>
    void add_at(const chrono::system_clock::time_point& abs_time, Func&& closure) {
        pool->submit_at(abs_time, std::move(closure));
    }

    template<class Func>
    void add_after(const chrono::system_clock::duration& rel_time, Func&& closure) {
        pool->submit_after(rel_time, std::move(closure));
    }

    virtual size_t uninitiated_task_count() const {
        return 0;
    }
};

namespace details {
    extern system_executor g_system_executor;
}

#endif