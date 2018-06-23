cmake_minimum_required(VERSION 2.6)

set(CMAKE_SYSTEM_NAME Generic)

project(teki C CXX)

set(UCI_NAME "\\\"${EXEC_NAME}\\\"")

set(CMAKE_C_FLAGS "-pipe -O3 -flto -DNDEBUG")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -pipe -DNAME=${UCI_NAME}")
if(EXTRA_CXX_FLAGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EXTRA_CXX_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")

find_package(Threads)

add_executable(teki main.cpp uci.cpp lookups.cpp position.cpp movegen.cpp
                     move.cpp search.cpp evaluate.cpp options.cpp mcts.cpp
                     syzygy/tbprobe.c)

target_link_libraries(teki "${CMAKE_THREAD_LIBS_INIT}")
if(EXTRA_LINK_FLAGS)
    set_target_properties(teki PROPERTIES LINK_FLAGS "${EXTRA_LINK_FLAGS}")
endif()
