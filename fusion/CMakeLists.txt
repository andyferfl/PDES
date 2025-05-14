cmake_minimum_required(VERSION 3.10)
project(Fusion_Library)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(include)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(include)

set(CORE_SOURCES
    lib/core/event.cpp
    lib/core/entity.cpp
    lib/core/logical_process.cpp
    lib/core/simulation_engine.cpp
    lib/core/simulation_manager.cpp
)

set(ALGORITHM_SOURCES
    lib/algorithms/sequential_engine.cpp
    lib/algorithms/null_messages_engine.cpp
    lib/algorithms/window_racer_engine.cpp
)

add_library(fusion SHARED
    ${CORE_SOURCES}
    ${ALGORITHM_SOURCES}
)

install(TARGETS fusion DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)
