cmake_minimum_required(VERSION 2.8)

project(executors)

MESSAGE( STATUS "CMAKE_CXX_COMPILER_ID: " ${CMAKE_CXX_COMPILER_ID} )

# define some folders
set(EXTR_DIR ${CMAKE_CURRENT_SOURCE_DIR})

MESSAGE( STATUS "EXTR_DIR: " ${EXTR_DIR} )

FIND_PACKAGE(Threads)

if (APPLE)
	add_definitions(-DEXTR_DEFINE_MISSING_STD_TYPES=1)
	set(EXTR_PLATFORM_DIR ${EXTR_DIR}/include/gcd)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++11 -ftemplate-depth=1024 ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list( APPEND CMAKE_CXX_FLAGS " -std=c++11 ${CMAKE_CXX_FLAGS}")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    list( APPEND CMAKE_CXX_FLAGS " /DUNICODE /D_UNICODE /bigobj ${CMAKE_CXX_FLAGS}")
endif()

include_directories(${EXTR_DIR}/ext/catch/include ${EXTR_DIR}/include ${EXTR_PLATFORM_DIR} ${EXTR_DIR}/test)

set(TEST_DIR ${EXTR_DIR}/test)

# define the sources of the self test
set(TEST_SOURCES
    ${TEST_DIR}/catch/test.cpp
)
add_executable(extr_test ${TEST_SOURCES})
TARGET_LINK_LIBRARIES(extr_test ${CMAKE_THREAD_LIBS_INIT})

# configure unit tests via CTest
enable_testing()

add_test(NAME RunTests COMMAND extr_test)

add_test(NAME ListTests COMMAND extr_test --list-tests)
set_tests_properties(ListTests PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ test case")

add_test(NAME ListTags COMMAND extr_test --list-tags)
set_tests_properties(ListTags PROPERTIES PASS_REGULAR_EXPRESSION "[0-9]+ tags")
