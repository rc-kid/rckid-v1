cmake_minimum_required(VERSION 3.10)
project(rcboy)

set (CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/common)

#add_subdirectory("rcboy/rpi/gamepad")
add_subdirectory("rcboy/rpi/test-radio")
add_subdirectory("rcboy/rpi/test-avr")