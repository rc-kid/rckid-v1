cmake_minimum_required(VERSION 3.10)
project(tools)

set (CMAKE_CXX_STANDARD 17)


file(GLOB_RECURSE SRC  "divider.cpp")
add_executable(divider ${SRC})
