cmake_minimum_required(VERSION 2.6)
project(TEKI CXX)

set(EXEC_NAME "teki")
set(UCI_NAME "\\\"${EXEC_NAME}\\\"")

set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -pipe -DNAME=${UCI_NAME}")

find_package(Threads)

add_executable(teki main.cpp uci.cpp lookups.cpp position.cpp movegen.cpp
    move.cpp search.cpp evaluate.cpp options.cpp globals.cpp)
target_link_libraries(teki "${CMAKE_THREAD_LIBS_INIT} -Wl,--no-undefined")
