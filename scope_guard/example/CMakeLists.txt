cmake_minimum_required(VERSION 3.16)
project(scope_guard)

set(CMAKE_CXX_STANDARD 23)

include_directories(../../../)

add_executable(scope_guard main.cpp)
