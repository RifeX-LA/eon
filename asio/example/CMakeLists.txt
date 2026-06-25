cmake_minimum_required(VERSION 3.16)
project(asio)

set(CMAKE_CXX_STANDARD 23)

find_package(Boost COMPONENTS asio REQUIRED)

include_directories(${Boost_INCLUDE_DIR} ../../..)

add_executable(asio main.cpp)
target_link_libraries(asio ${Boost_LIBRARIES})