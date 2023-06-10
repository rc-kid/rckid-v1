project(rbench)

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
#pkg_check_modules(EGL REQUIRED egl)
#pkg_check_modules(GLES2 REQUIRED glesv2)    
find_library(M_LIBRARY libm.so)
find_library(DL_LIBRARY libdl.so)
find_library(RT_LIBRARY librt.so)


# libraries required for raylib

#find_package(OpenGL REQUIRED)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
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
add_executable(rbench ${SRC})
target_link_libraries(rbench Threads::Threads)
target_link_libraries(rbench
    ${CMAKE_SOURCE_DIR}/../rckid-raylib/src/libraylib.a 
    ${M_LIBRARY} 
    ${DL_LIBRARY} 
    ${RT_LIBRARY})

if(DEFINED ARCH_RPI)
    target_link_libraries(rbench
        ${GLES_LIBRARY}
        ${EGL_LIBRARY}
        ${BCM_LIBRARY}
        ${ATOMIC_LIBRARY}
        ${BCM_HOST_LIBRARY}
        # the following libraries are needed for the KMS drivers
        # ${DRM_LIBRARY}
        # ${GBM_LIBRARY}
        )
else()
    target_link_libraries(rbench ${GL_LIBRARIES})
endif()
