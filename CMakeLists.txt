cmake_minimum_required(VERSION 3.10)
project(rcboy)

set (CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

#add_subdirectory("rcboy/rpi")
add_subdirectory("rcboy/rpi-raylib")
add_subdirectory("avr-i2c-bootloader")