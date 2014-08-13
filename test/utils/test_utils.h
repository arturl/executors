#pragma once

struct test_base
{
    virtual void run() = 0;
    virtual void start_test() = 0;
    virtual void end_test() = 0;
};

class test_list
{
    std::vector<test_base*> tests;
public:
    void add(test_base*test) {
        tests.push_back(test);
    }
    void run_all() {
        for (auto & t : tests) {
            t->start_test();
            t->run();
//            t->end_test();
        }
    }
};

struct console_restorer {
    CONSOLE_SCREEN_BUFFER_INFO m_originalConsoleInfo;
    console_restorer()
    {
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &m_originalConsoleInfo);
    }
    ~console_restorer()
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), m_originalConsoleInfo.wAttributes);
    }
};

static void ChangeConsoleTextColorToGreen()
{
#ifdef WIN32
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 0x0002 | 0x0008);
#else
    std::cout << "\033[1;32m";
#endif
}

#define TEST_FIXTURE(test_name, enabled) struct test_##test_name : public test_base { \
    test_##test_name() {                            \
        if(enabled) g_tests.add(this);              \
    }                                               \
    virtual void start_test() {                     \
        console_restorer _;                         \
        ChangeConsoleTextColorToGreen();            \
        printf("** Start '%s'\n", #test_name);      \
    }                                               \
    virtual void end_test() {                       \
        console_restorer _;                         \
        ChangeConsoleTextColorToGreen();            \
        printf("** End test '%s'\n", #test_name);   \
    }                                               \
    virtual void test_##test_name::run();           \
}inst_test_##test_name;                             \
    void test_##test_name::run()


#define TEST_FILE test_list g_tests
#define RUN_TESTS g_tests.run_all()
