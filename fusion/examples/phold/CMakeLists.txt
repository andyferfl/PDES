cmake_minimum_required(VERSION 3.10)
project(PHOLD_model)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(FUSION_INSTALL_DIR NAMES libfusion.so HINTS "../../install")

include_directories(${FUSION_INSTALL_DIR}/include)
link_directories(${FUSION_INSTALL_DIR}/lib)

add_executable(phold_simulation main.cpp phold.hpp phold.cpp)
target_link_libraries(phold_simulation PRIVATE fusion)
