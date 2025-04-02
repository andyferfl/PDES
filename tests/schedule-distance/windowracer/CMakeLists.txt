cmake_minimum_required(VERSION 3.8)
project(window_racer)

set(CMAKE_CXX_STANDARD 17)
configure_file(Makefile.in Makefile @ONLY)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
add_executable(window_racer window_racer.cpp event.cpp)
target_link_libraries(window_racer PRIVATE Threads::Threads)