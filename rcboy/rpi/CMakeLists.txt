project(rcboy)

if(DEFINED ARCH_RPI)
else()
    add_definitions(-DARCH_MOCK)
endif()

# threads are needed

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# libevdev is required for simulating the gamepad

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_EVDEV libevdev REQUIRED)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)
include_directories(${EVDEV_INCLUDE_DIR})

# QT is required for the GUI 

find_package(Qt5 COMPONENTS Widgets Gamepad REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(DEFINED ARCH_RPI)
    message(STATUS "Building for raspberry pi")
    add_definitions(-DARCH_RPI)
    find_path(PIGPIO_INCLUDE_DIR 
            NAMES pigpio.h pigpiod_if.h pigpiod_if2.h
            HINTS /usr/local/include)        
    find_library(PIGPIO_LIBRARY 
            NAMES libpigpio.so
            HINTS /usr/local/lib)
    include_directories(${PIGPIO_INCLUDE_DIR})
else()
    message(STATUS "Mockup build")
    add_definitions(-DARCH_MOCK)
endif()

file(GLOB_RECURSE SRC  *.cpp *.h)
add_executable(rcboy ${SRC})
target_link_libraries(rcboy ${EVDEV_LIBRARY})
target_link_libraries(rcboy Qt5::Widgets)
target_link_libraries(rcboy Threads::Threads)


if(DEFINED ARCH_RPI)
    target_link_libraries(rcboy ${PIGPIO_LIBRARY})
endif()
