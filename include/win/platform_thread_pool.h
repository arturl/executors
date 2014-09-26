#include "thread_helper.h"

using namespace std;

class thread_pool {
private:
    shared_ptr<details::functional_timer_pool> pool;
public:
    explicit thread_pool(int N) : pool(new details::functional_timer_pool(N)) {
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

#if 0 // debugging
    thread_pool(thread_pool const& other) {
        this->pool = other.pool;
        printf("copy ctor thread_pool\n");
    }
    thread_pool(thread_pool&& other) {
        this->pool = std::move(other.pool);
        printf("move ctor thread_pool\n");
    }
#endif
};
