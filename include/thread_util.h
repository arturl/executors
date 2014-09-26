#ifndef THREAD_UTIL
#define THREAD_UTIL

#include <functional>
#include <chrono>
#include <ctime>

template <typename Type, typename Traits>
class unique_handle {
    struct boolean_struct { int member; };
    typedef int boolean_struct::* boolean_type;

    unique_handle(unique_handle const &);
    Type m_value;

    bool operator==(unique_handle const &);
    bool operator!=(unique_handle const &);
    unique_handle & operator=(unique_handle const &);

    void close() throw() {
        if (*this) {
            Traits::close(m_value);
        }
    }

public:

    explicit unique_handle(Type value = Traits::invalid()) throw() : m_value(value) {}

    ~unique_handle() throw() {
        close();
    }

    unique_handle(unique_handle && other) throw() : m_value(other.release()) {}

    Type get() const throw() {
        return m_value;
    }

    bool reset(Type value = Traits::invalid()) throw() {
        if (m_value != value) {
            close();
            m_value = value;
        }

        return *this;
    }

    Type release() throw() {
        auto value = m_value;
        m_value = Traits::invalid();
        return value;
    }

    /*Miscellaneous functions */
    operator boolean_type() const throw() {
        return Traits::invalid() != m_value ? &boolean_struct::member : nullptr;
    }

    unique_handle & operator=(unique_handle && other) throw() {
        reset(other.release());
        return *this;
    }
};

#endif