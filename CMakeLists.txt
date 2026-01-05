cmake_minimum_required(VERSION 3.20)
project(thread_pool VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# sanitizer support
option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(thread_pool PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(thread_pool PRIVATE -fsanitize=address,undefined)
endif()


# library
add_library(thread_pool
    src/thread_pool.cpp
)

target_include_directories(thread_pool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
) 

# fetch GTest (builds with our flags)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)


# tests
enable_testing()
find_package(GTest REQUIRED)

add_executable(thread_pool_test tests/thread_pool_test.cpp)
target_link_libraries(thread_pool_test PRIVATE thread_pool GTest::gtest_main)
add_test(NAME thread_pool_test COMMAND thread_pool_test)

# example
add_executable(basic_usage examples/basic_usage.cpp)
target_link_libraries(basic_usage PRIVATE thread_pool)