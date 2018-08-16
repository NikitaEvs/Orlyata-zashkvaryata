cmake_minimum_required(VERSION 2.8.3)
project(cppserver)
set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS  "-lboost_system -lssl -pthread -lboost_filesystem -lboost_thread ${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS  "-lboost_system -lssl -pthread -lboost_filesystem -lboost_thread ${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}")
find_package(Boost COMPONENTS system)
include_directories(
  thirdparty
)
add_executable(cppserver src/server.cpp)
add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
