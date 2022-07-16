project(test-avr)

if(DEFINED ARCH_RPI)
    add_definitions(-DARCH_RPI)
else()
    add_definitions(-DARCH_MOCK)
endif()

if(DEFINED ARCH_RPI)
    message(STATUS "Building for raspberry pi")
    find_path(PIGPIO_INCLUDE_DIR 
            NAMES pigpio.h pigpiod_if.h pigpiod_if2.h
            HINTS /usr/local/include)        
    find_library(PIGPIO_LIBRARY 
            NAMES libpigpio.so
            HINTS /usr/local/lib)
    include_directories(${PIGPIO_INCLUDE_DIR})
endif()


file(GLOB_RECURSE SRC  *.cpp *.h)
add_executable(test-avr ${SRC})

if(DEFINED ARCH_RPI)
    target_link_libraries(test-avr ${PIGPIO_LIBRARY})
endif()
