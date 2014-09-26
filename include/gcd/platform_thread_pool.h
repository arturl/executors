#pragma once

#include <atomic>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include <executor.h>
#include <thread_util.h>

#include <dispatch/dispatch.h>

struct dispatch_queue_traits
{
    static dispatch_queue_t invalid() throw()
    {
        return dispatch_queue_t{};
    }

    static void close(dispatch_queue_t value) throw()
    {
        dispatch_release(value);
    }
};
typedef unique_handle<dispatch_queue_t, dispatch_queue_traits> unique_dispatch_queue;

struct dispatch_group_traits
{
    static dispatch_group_t invalid() throw()
    {
        return dispatch_group_t{};
    }

    static void close(dispatch_group_t value) throw()
    {
        dispatch_release(value);
    }
};
typedef unique_handle<dispatch_group_t, dispatch_group_traits> unique_dispatch_group;

struct dispatch_source_traits
{
    static dispatch_source_t invalid() throw()
    {
        return dispatch_source_t{};
    }

    static void close(dispatch_source_t value) throw()
    {
        dispatch_release(value);
    }
};
typedef unique_handle<dispatch_source_t, dispatch_source_traits> unique_dispatch_source;

inline uint64_t absolute_to_relative_ns(const chrono::system_clock::time_point& abs_time) {
    using namespace std::chrono;
    return std::max(0LL, duration_cast<nanoseconds>(abs_time - system_clock::now()).count());
}

inline uint64_t relative_to_relative_ns(const chrono::system_clock::duration& rel_time) {
    using namespace std::chrono;
    typedef std::chrono::duration<uint64_t, std::ratio<1, NSEC_PER_SEC>> nanosec;

    return duration_cast<nanosec>(rel_time).count();
}

/* For passing std::function as a void pointer */
template <class derived>
class fnc_wrapper_interface {
    void run() {
        static_cast<derived*>(this)->start();
    }
};

template<class Pool>
class fnc_wrapper : fnc_wrapper_interface<fnc_wrapper<Pool>> {
    function<void()> m_ptr;
    Pool& m_pool;
public:
    template<class Func>
    fnc_wrapper(Func&& wrapped, Pool& pool) : m_ptr(std::forward<Func>(wrapped)), m_pool(pool) {}
    void run() { m_ptr(); }
    Pool& pool() { return m_pool; }
};

template<class Pool>
class time_fnc_wrapper : fnc_wrapper_interface<time_fnc_wrapper<Pool>> {
    function<void()> ptr;
    Pool& thepool;
public:
    unique_dispatch_source dispatch_in_time;
    ~time_fnc_wrapper(){
        dispatch_source_cancel(dispatch_in_time.get());
        dispatch_group_leave(thepool.dispatch_group.get());
        dispatch_release(thepool.dispatch_group.get());
    }
    template<class Func>
    time_fnc_wrapper(uint64_t duration_ns, Func&& wrapped, Pool& pool) : 
        ptr(std::forward<Func>(wrapped)), 
        thepool(pool),
        dispatch_in_time(dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, thepool.dispatch_queue.get())) {
        dispatch_retain(thepool.dispatch_group.get());
        dispatch_group_enter(thepool.dispatch_group.get());
        dispatch_source_set_timer(dispatch_in_time.get(), dispatch_walltime(nullptr, duration_ns), 0, 0);
    }
    void run() { ptr(); }
    Pool& pool() { return thepool; }
};

class thread_pool {
private:

    struct pool_group {
        unique_dispatch_queue dispatch_queue;
        unique_dispatch_group dispatch_group;
        ~pool_group() 
        {
            dispatch_group_wait(dispatch_group.get(), DISPATCH_TIME_FOREVER);
        }
        explicit pool_group(int N, dispatch_queue_t q) : 
            dispatch_queue(q),
            dispatch_group(dispatch_group_create())
        {
            if(N == 1) {
                dispatch_retain(dispatch_queue.get());
            }
        }

        static void callback(void* context) {
            auto wrapper = reinterpret_cast<fnc_wrapper<pool_group> *>(context);
            wrapper->run();
            delete wrapper;
        }
        static void time_callback(void* context) {
            auto wrapper = reinterpret_cast<time_fnc_wrapper<pool_group> *>(context);
            wrapper->run();
            delete wrapper;
        }

        template<class Func>
        void submit(Func&& closure) {
            fnc_wrapper<pool_group> * wrapper = new fnc_wrapper<pool_group>(std::forward<Func>(closure), *this);
            dispatch_group_async_f(dispatch_group.get(), dispatch_queue.get(), wrapper, callback);
        }
        template<class Func>
        void submit_at(const chrono::system_clock::time_point& abs_time, Func&& closure) {
            auto duration_ns = absolute_to_relative_ns(abs_time);
            time_fnc_wrapper<pool_group> * wrapper = new time_fnc_wrapper<pool_group>(
                duration_ns, 
                std::forward<Func>(closure), 
                *this);
            dispatch_set_context(wrapper->dispatch_in_time.get(), wrapper);
            dispatch_source_set_event_handler_f(wrapper->dispatch_in_time.get(), time_callback);
            dispatch_resume(wrapper->dispatch_in_time.get());
        }
        template<class Func>
        void submit_after(const chrono::system_clock::duration& rel_time, Func&& closure) {
            auto duration_ns = relative_to_relative_ns(rel_time);
            time_fnc_wrapper<pool_group> * wrapper = new time_fnc_wrapper<pool_group>(
                duration_ns, 
                std::forward<Func>(closure), 
                *this);
            dispatch_set_context(wrapper->dispatch_in_time.get(), wrapper);
            dispatch_source_set_event_handler_f(wrapper->dispatch_in_time.get(), time_callback);
            dispatch_resume(wrapper->dispatch_in_time.get());
        }
    };
    shared_ptr<pool_group> pool;

public:
    thread_pool() : 
        pool(std::make_shared<pool_group>(0, 
            dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0))) {
    }
    thread_pool(int N) : 
        pool(std::make_shared<pool_group>(N, N == 1 ? dispatch_queue_create("serial executor pool", DISPATCH_QUEUE_SERIAL) :
            dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0))) {
    }

    template<class Func>
    void add(Func&& closure) {
        pool->submit(std::forward<Func>(closure));
    }

    template<class Func>
    void add_at(const chrono::system_clock::time_point& abs_time, Func&& closure) {
        pool->submit_at(abs_time, std::forward<Func>(closure));
    }

    template<class Func>
    void add_after(const chrono::system_clock::duration& rel_time, Func&& closure) {
        pool->submit_after(rel_time, std::forward<Func>(closure));
    }

    virtual size_t uninitiated_task_count() const {
        return 0;
    }
};
