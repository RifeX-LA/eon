cmake_minimum_required(VERSION 3.26)
project(spark)

set(CMAKE_CXX_STANDARD 23)

include_directories(../../../)

add_executable(spark main.cpp)
