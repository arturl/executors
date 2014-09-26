#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <memory>

#include <executor.h>
#include <serial_executor.h>
#include <system_executor.h>
#include <thread_per_task_executor.h>
#include <thread_pool.h>
#include <utils/semaphore.h>

SCENARIO("thread_pool is not serial", "[thread_pool][executor]"){
    GIVEN("a thread_pool"){
        WHEN("two tasks use semaphores"){
            std::atomic<int> completed{0};
            {
                // Ensure that thread_pool with level of concurrency >1 can run tasks concurrently

                utils::semaphore task1_started(1);
                utils::semaphore task2_started(1);

                thread_pool tp;

                tp.add([&] {
                    printf("task 1 started\n");
                    task1_started.notify();
                    task2_started.wait();
                    printf("task 1 end\n");
                    ++completed;
                });
                tp.add([&] {
                    task1_started.wait();
                    printf("task 2 start\n");
                    task2_started.notify();
                    printf("task 2 end\n");
                    ++completed;
                });
            }

            THEN("all must finish"){
                REQUIRE(completed == 2);
            }
        }
    }
}

SCENARIO("thread_pool(1) is serial", "[thread_pool][executor]"){
    GIVEN("a thread_pool"){
        WHEN("two tasks use bools"){

            // Ensure that thread_pool with level of concurrency 1 runs tasks serially

            std::atomic_bool task1_is_running{ false };
            std::atomic_bool task2_is_running{false };
            std::atomic_bool task1_passed{ false };
            std::atomic_bool task2_passed{false };
            std::chrono::milliseconds sleep(100);

            std::atomic<int> completed{0};
            {
                thread_pool tp(1);

                tp.add([&] {
                    printf("task 1 started\n");
                    task1_is_running = true;
                    std::this_thread::sleep_for(sleep);
                    task1_is_running = false;
                    task1_passed = !task2_is_running;
                    printf("task 1 end\n");
                    ++completed;
                });
                tp.add([&] {
                    printf("task 2 started\n");
                    task2_is_running = true;
                    task2_passed = !task1_is_running;
                    task2_is_running = false;
                    printf("task 2 end\n");
                    ++completed;
                });
            }

            THEN("all must finish"){
                REQUIRE(completed == 2);
            }
            THEN("all ran serially"){
                REQUIRE(task1_passed);
                REQUIRE(task2_passed);
            }
        }
    }
}


SCENARIO("serial_executor", "[serial_executor][executor]"){
    GIVEN("a serial_executor"){
        WHEN("three tasks are added"){
            std::atomic_bool task1_passed{ false };
            std::atomic_bool task2_passed{ false };
            std::atomic_bool task3_passed{ false };
            std::atomic<int> completed{0};
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
                    task1_passed = !task2_is_running && !task3_is_running;
                    printf("1 done! ");
                    ++completed;
                });
                se.add([&] {
                    printf("2...");
                    task2_is_running = true;
                    std::this_thread::sleep_for(sleep);
                    task2_is_running = false;
                    task2_passed = !task1_is_running && !task3_is_running;
                    printf("2 done! ");
                    ++completed;
                });
                se.add([&] {
                    printf("3...");
                    task3_is_running = true;
                    std::this_thread::sleep_for(sleep);
                    task3_is_running = false;
                    task3_passed = !task1_is_running && !task2_is_running;
                    printf("3 done!\n");
                    ++completed;
                });
            }
            THEN("all must finish"){
                REQUIRE(completed == 3);
            }
            THEN("all ran serially"){
                REQUIRE(task1_passed);
                REQUIRE(task2_passed);
                REQUIRE(task3_passed);
            }
        }
    }
}

SCENARIO("abstract_executor", "[abstract_executor][executor]"){
    GIVEN("a thread_pool"){
        WHEN("three tasks are added"){
            std::atomic<int> completed{0};
            {
                thread_pool tp;
                abstract_executor ae(tp);

                ae.add([&] { 
                    printf("hi from abstract_executor!\n"); 
                    ++completed;
                });

                abstract_executor_ref ae2 = &tp;

                ae2.add([&] {
                    printf("hi from abstract_executor_ref!\n");
                    ++completed;
                });

                abstract_executor_ref ae3 = &ae;

                ae3.add([&] {
                    printf("hi from abstract_executor_ref, again!\n");
                    ++completed;
                });
            }

            THEN("all must finish"){
                REQUIRE(completed == 3);
            }
        }
    }
}

SCENARIO("system_executor", "[system_executor][executor]"){
    GIVEN("a system_executor"){
        WHEN("1 task added"){
            std::atomic<int> completed{0};
            {
                system_executor& se = system_executor::get_system_executor();
                
                utils::semaphore s(1);

                se.add([&] {
                    ++completed;
                    s.notify();
                });

                s.wait();
            }

            THEN("all must finish"){
                REQUIRE(completed == 1);
            }
        }
    }
}

SCENARIO("thread_per_task_executor", "[thread_per_task_executor][executor]"){
    GIVEN("a thread_per_task_executor"){
        WHEN("2 tasks added"){
            std::thread::id id1, id2;
            std::atomic<int> completed{0};
            {
                thread_per_task_executor& tpt = thread_per_task_executor::get_thread_per_task_executor();

                utils::semaphore s(2);

                tpt.add([&] {
                    id1 = std::this_thread::get_id();
                    ++completed;
                    s.notify();
                });
                tpt.add([&] {
                    id2 = std::this_thread::get_id();
                    ++completed;
                    s.notify();
                });

                s.wait();
            }

            THEN("all must finish"){
                REQUIRE(completed == 2);
            }
            THEN("2 threads created"){
                REQUIRE(id1 != id2);
            }
        }
    }
}

SCENARIO("thread_pool in relative time", "[time][thread_pool][executor]"){
    GIVEN("a thread_pool"){
        WHEN("one timed task"){
            using namespace std::chrono;
            typedef steady_clock clock;

            std::atomic<int> completed{0};
            auto start = clock::now();
            auto finish = start;
            {
                thread_pool tp;

                tp.add_after(milliseconds(200), [&] {
                    finish = clock::now();
                    printf("relative timed task end\n");
                    ++completed;
                });
            }

            THEN("must only run once"){
                REQUIRE(completed == 1);
            }
            THEN("must must run at the specified time"){
                auto required = duration_cast<milliseconds>(milliseconds(200)).count();
                REQUIRE(start < finish);
                auto actual = duration_cast<milliseconds>(finish-start).count();
                REQUIRE(actual > 0);
                auto ratio = float(required) / actual;
                REQUIRE(ratio < 1.05);
                REQUIRE(ratio > 0.95);
            }
        }
    }
}

SCENARIO("thread_pool in absolute time", "[time][thread_pool][executor]"){
    GIVEN("a thread_pool"){
        WHEN("one timed task"){
            using namespace std::chrono;
            typedef steady_clock clock;

            auto duration = milliseconds(200);
            std::atomic<int> completed{0};
            auto start = clock::now();
            auto finish = start;
            {
                thread_pool tp;

                tp.add_at(system_clock::now() + duration, [&] {
                    finish = clock::now();
                    printf("absolute timed task end\n");
                    ++completed;
                });
            }

            THEN("must only run once"){
                REQUIRE(completed == 1);
            }
            THEN("must must run at the specified time"){
                auto required = duration_cast<milliseconds>(duration).count();
                REQUIRE(start < finish);
                auto actual = duration_cast<milliseconds>(finish-start).count();                
                REQUIRE(actual > 0);
                auto ratio = float(required) / actual;
                REQUIRE(ratio < 1.05);
                REQUIRE(ratio > 0.95);
            }
        }
    }
}
