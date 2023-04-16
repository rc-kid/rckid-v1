cmake_minimum_required(VERSION 3.10)
project(rckid)

set (CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory("tools")
add_subdirectory("rckid/rpi")
add_subdirectory("avr-i2c-bootloader")
add_subdirectory("include/utils")