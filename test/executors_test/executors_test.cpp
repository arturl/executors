// executors_test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "executor.h"
#include "thread_pool.h"
#include "serial_executor.h"
#include "thread_per_task_executor.h"
#include "system_executor.h"
#include "..\utils\semaphore.h"
#include "..\utils\test_utils.h"

TEST_FILE;

#define ENABLE_THREAD_POOL_TESTS        1
#define ENABLE_SERIAL_EXECUTOR_TESTS    1
#define ENABLE_ABSTRACT_EXECUTOR_TESTS  1
#define ENABLE_DEFAULT_EXECUTOR_TESTS    1

TEST_FIXTURE(test_thread_pool_01, ENABLE_THREAD_POOL_TESTS)
{
    // Ensure that thread_pool with level of concurrency >1 can run tasks concurrently

    utils::semaphore task1_started(1);
    utils::semaphore task2_started(1);
    thread_pool tp(2);

    tp.add([&] {
        printf("task 1 started\n");
        task1_started.notify();
        task2_started.wait();
        printf("task 1 end\n");
    });
    tp.add([&] {
        task1_started.wait();
        printf("task 2 start\n");
        task2_started.notify();
        printf("task 2 end\n");
    });
}

TEST_FIXTURE(test_thread_pool_02, ENABLE_THREAD_POOL_TESTS)
{
    // Ensure that thread_pool with level of concurrency 1 runs tasks serially

    std::atomic_bool task1_is_running{ false };
    std::atomic_bool task2_is_running{false };
    std::chrono::milliseconds sleep(100);

    thread_pool tp(1);

    tp.add([&] {
        printf("task 1 started\n");
        task1_is_running = true;
        std::this_thread::sleep_for(sleep);
        task1_is_running = false;
        assert(!task2_is_running);
        printf("task 1 end\n");
    });
    tp.add([&] {
        printf("task 2 started\n");
        task2_is_running = true;
        assert(!task1_is_running);
        task2_is_running = false;
        printf("task 2 end\n");
    });
}

TEST_FIXTURE(test_serial_executor_01, ENABLE_SERIAL_EXECUTOR_TESTS)
{
    // Ensure that serial_executor runs tasks serially, even on a concurrent thread pool

    std::atomic_bool task1_is_running{ false };
    std::atomic_bool task2_is_running{ false };
    std::atomic_bool task3_is_running{ false };
    std::chrono::milliseconds sleep(100);

    thread_pool tp(8);
    serial_executor se(&tp);
    se.add([&] {
        printf("1...");
        task1_is_running = true;
        std::this_thread::sleep_for(sleep);
        task1_is_running = false;
        assert(!task2_is_running);
        assert(!task3_is_running);
        printf("1 done! ");
    });
    se.add([&] {
        printf("2...");
        task2_is_running = true;
        std::this_thread::sleep_for(sleep);
        task2_is_running = false;
        assert(!task1_is_running);
        assert(!task3_is_running);
        printf("2 done! ");
    });
    se.add([&] {
        printf("3...");
        task3_is_running = true;
        std::this_thread::sleep_for(sleep);
        task3_is_running = false;
        assert(!task1_is_running);
        assert(!task2_is_running);
        printf("3 done!\n");
    });
}

TEST_FIXTURE(test_abstract_executor, ENABLE_ABSTRACT_EXECUTOR_TESTS)
{
    thread_pool tp(8);
    abstract_executor ae(tp);

    ae.add([] { 
        printf("hi from abstract_executor!\n"); 
    });

    abstract_executor_ref ae2 = &tp;

    ae2.add([] {
        printf("hi from abstract_executor_ref!\n");
    });

    abstract_executor_ref ae3 = &ae;

    ae3.add([] {
        printf("hi from abstract_executor_ref, again!\n");
    });

}

TEST_FIXTURE(test_system_executor, ENABLE_DEFAULT_EXECUTOR_TESTS)
{
    system_executor& se = system_executor::get_system_executor();
    
    utils::semaphore s(1);

    se.add([&] {
        s.notify();
    });

    s.wait();
}

TEST_FIXTURE(test_thread_per_task_executor_01, ENABLE_DEFAULT_EXECUTOR_TESTS)
{
    thread_per_task_executor& tpt = thread_per_task_executor::get_thread_per_task_executor();

    std::thread::id id1, id2;

    tpt.add([&] {
        id1 = std::this_thread::get_id();
    });
    tpt.add([&] {
        id2 = std::this_thread::get_id();
    });
    assert(id1 != id2);

}

int main()
{
    RUN_TESTS;
    return 0;
}

