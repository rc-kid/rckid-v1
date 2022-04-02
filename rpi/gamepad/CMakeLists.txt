project(gamepad-driver)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_EVDEV libevdev REQUIRED)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)
include_directories(${EVDEV_INCLUDE_DIR})


#find_path(PIGPIO_INCLUDE_DIR 
#        NAMES pigpio.h pigpiod_if.h pigpiod_if2.h
#        HINTS /usr/local/include)        
#find_library(PIGPIO_LIBRARY 
#        NAMES libpigpio.so
#        HINTS /usr/local/lib)
#include_directories ${PIGPIO_INCLUDE_DIR})


file(GLOB_RECURSE SRC  *.cpp *.h)
add_executable(gamepad-driver ${SRC})
target_link_libraries(gamepad-driver ${EVDEV_LIBRARY})
#target_link_libraries(gamepad-driver ${PIGPIO_LIBRARY})
