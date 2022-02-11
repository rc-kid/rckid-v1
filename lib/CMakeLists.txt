cmake_minimum_required(VERSION 3.12)

project(libs)

file(GLOB_RECURSE SRC "libopus/*.cpp" "libopus/*.h" "libopus/*.c")
add_library(libopus ${SRC})

file(GLOB_RECURSE SRC "libogg/*.cpp" "libogg/*.h" "libogg/*.c")
add_library(libogg ${SRC})

file(GLOB_RECURSE SRC "opusfile/*.cpp" "opusfile/*.h" "opusfile/*.c")
add_library(libopusfile ${SRC})