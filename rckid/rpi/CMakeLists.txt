project(rckid)

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

# opus library for encoding and decoding audio data
find_library(OPUS_LIBRARY libopus.so.0)
find_path(OPUS_INCLUDE_DIR opus/opus.h HINTS /usr/include)
include_directories(${OPUS_INCLUDE_DIR})

#pkg_check_modules(EGL REQUIRED egl)
#pkg_check_modules(GLES2 REQUIRED glesv2)    
find_library(M_LIBRARY libm.so)
find_library(DL_LIBRARY libdl.so)
find_library(RT_LIBRARY librt.so)



# libraries required for raylib

#find_package(OpenGL REQUIRED)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../..)
include_directories(${CMAKE_SOURCE_DIR}/../rckid-raylib/src)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

if(DEFINED ARCH_RPI)
    message(STATUS "Building for raspberry pi")
    add_definitions(-DARCH_RPI)
    find_library(GLES_LIBRARY libbrcmGLESv2.so HINTS /opt/vc/lib)
    find_library(EGL_LIBRARY libbrcmEGL.so HINTS /opt/vc/lib)
    find_library(BCM_LIBRARY libbcm_host.so HINTS /opt/vc/lib)
    find_library(ATOMIC_LIBRARY libatomic.so.1 HINTS /usr/lib/arm-linux-gnueabihf)
    find_library(BCMHOST_LIBRARY libbcm_host.so HINTS /opt/vc/lib)
    # the following libraries are needed for the KMS drivers
    # find_library(DRM_LIBRARY libdrm.so HINTS /opt/vc/lib)
    # find_library(GBM_LIBRARY libgbm.so HINTS /opt/vc/lib)
        #find_path(PIGPIO_INCLUDE_DIR 
    #        NAMES pigpio.h pigpiod_if.h pigpiod_if2.h
    #        HINTS /usr/local/include)        
    include_directories(/opt/vc/include)
else()
    message(STATUS "Mockup build")
    add_definitions(-DARCH_MOCK)
    pkg_check_modules(GL REQUIRED gl)
    endif()

file(GLOB_RECURSE SRC  *.cpp *.h)
add_executable(rckid ${SRC})
target_link_libraries(rckid libi2cprogrammer)
target_link_libraries(rckid ${EVDEV_LIBRARY} ${OPUS_LIBRARY})
target_link_libraries(rckid Threads::Threads uuid stdc++fs)
target_link_libraries(rckid
    ${CMAKE_SOURCE_DIR}/../rckid-raylib/src/libraylib.a 
    ${M_LIBRARY} 
    ${DL_LIBRARY} 
    ${RT_LIBRARY})

if(DEFINED ARCH_RPI)
    add_compile_options(-Wno-psabi)
    target_link_libraries(rckid
        ${GLES_LIBRARY}
        ${EGL_LIBRARY}
        ${BCM_LIBRARY}
        ${ATOMIC_LIBRARY}
        ${BCM_HOST_LIBRARY}
        # the following libraries are needed for the KMS drivers
        # ${DRM_LIBRARY}
        # ${GBM_LIBRARY}
        wiringPi)
else()
    target_link_libraries(rckid ${GL_LIBRARIES})
endif()
