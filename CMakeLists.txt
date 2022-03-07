cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(${CMAKE_SOURCE_DIR}/sdk/pico_sdk_init.cmake)
# Initialize the SDK
pico_sdk_init()

project(picoboy C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


add_subdirectory("lib")

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)

add_compile_options(-DNO_PICO_LED=1)

file(GLOB_RECURSE SRC "src/*.cpp" "src/*.h")

add_executable(picoboy ${SRC})

pico_generate_pio_header(picoboy ${CMAKE_SOURCE_DIR}/src/graphics/ili9341.pio)
pico_generate_pio_header(picoboy ${CMAKE_SOURCE_DIR}/src/audio/i2s.pio)

# pull in common dependencies
target_link_libraries(picoboy
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_spi
    hardware_dma
    libopus
    libogg
    libopusfile
    FatFs_SPI
)

# create map/bin/hex file etc.
pico_add_extra_outputs(picoboy)

# add url via pico_set_program_url
#example_auto_set_url(blink)
