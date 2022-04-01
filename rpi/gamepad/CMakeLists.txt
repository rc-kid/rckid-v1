project(gamepad-driver)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PC_EVDEV libevdev REQUIRED)

find_path(EVDEV_INCLUDE_DIR libevdev/libevdev.h
          HINTS ${PC_EVDEV_INCLUDE_DIRS} ${PC_EVDEV_INCLUDEDIR})
find_library(EVDEV_LIBRARY
        NAMES evdev libevdev)

include_directories(${EVDEV_INCLUDE_DIR})

add_executable(gamepad-driver main.cpp)
target_link_libraries(gamepad-driver ${EVDEV_LIBRARY})