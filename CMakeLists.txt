cmake_minimum_required(VERSION 3.11)
project(SSKinect)

# optimization for dev environments
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# project version
set(MAJOR 0)
set(MINOR 1)
set(REMAP_VERSION ${MAJOR}.${MINOR})

# CXX and C versions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS "-pthread")

# cmake modules path spec
list(APPEND CMAKE_MODULE_PATH "$ENV{CMAKE_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# cmake binary path spec
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# build options
option(EXECUTE_TARGET "Execute Project Target" ON)                 # default OFF
option(BUILD_SHARED_LIBS "Build Shared Library" ON)                # default ON
option(OPTIMIZE_PROJECT "Tidy up and optimize source code" OFF)    # default OFF
option(INITIALIZE_K4A_SDK "Initialize the kinect SDK project" OFF) # default ON

if(OPTIMIZE_PROJECT)
    if(UNIX AND NOT APPLE)
        set(CMAKE_CXX_CLANG_TIDY
            clang-tidy -p pwd
            --checks=-*,readability-*,clang-analyzer-*,performance-*)
    endif()
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# dependencies
find_package(Git REQUIRED)
find_package(glog REQUIRED)
find_package(OpenCV REQUIRED)
find_package(gflags REQUIRED)
find_package (Eigen3 3.3 REQUIRED NO_MODULE)

# GLIB & GST
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
find_package(PkgConfig REQUIRED)
if ( NOT (PKGCONFIG_FOUND))
    message(FATAL_ERROR "Please Install PPkgConfig")
endif()
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GST REQUIRED
    gstreamer-1.0>=1.8
    gstreamer-video-1.0>=1.4
    gstreamer-app-1.0>=1.4
    )
if ( NOT (GST_FOUND))
    message(FATAL_ERROR "Please Install Gstreamer Dev")
endif()

set(ENV{PKG_CONFIG_PATH})

# main project include paths
set(PROJECT_DIR ${CMAKE_CURRENT_LIST_DIR})
set(EXT_DIR ${PROJECT_DIR}/external)

# initialize Azure-Kinect-Sensor-SDK submodule project
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    if(INITIALIZE_K4A_SDK)
        execute_process(
            COMMAND /bin/bash ./scripts/init_k4a_sdk.sh -j 12
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
    endif()
endif()

# after SDK initialization setup K4A (kinect SDK) paths
set(K4A_SDK ${EXT_DIR}/Azure-Kinect-Sensor-SDK)
set(K4A_LIBRARY ${K4A_SDK}/build)
set(K4A_VERSION ${K4A_SDK}/build/src/sdk/include)
set(K4A_INCLUDE ${K4A_SDK}/include)

set(RANSAC_INCLUDE_DIR ${PROJECT_DIR}/include)


# set-up target
add_executable(SSKinect
    ${PROJECT_DIR}/Camera.cpp
    ${PROJECT_DIR}/KinectAzure.cpp
    ${PROJECT_DIR}/SUR40.cpp
    ${PROJECT_DIR}/V4L2.cpp
    ${PROJECT_DIR}/k4a.cpp
    #${PROJECT_DIR}/kinect.cpp
    #${PROJECT_DIR}/realsense.cpp
    ${PROJECT_DIR}/Camera.cpp
    ${PROJECT_DIR}/Camera.cpp
    ${PROJECT_DIR}/timer.cpp
    )

# set-up target include directories
target_include_directories(SSKinect PRIVATE
    ${K4A_VERSION}
    ${K4A_INCLUDE}
    ${PROJECT_DIR}
    ${GST_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${RANSAC_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    )

# link libraries
target_link_libraries(SSKinect
    glog
    gflags
    Eigen3::Eigen
    ${OpenCV_LIBS}
    ${GLIB_LDFLAGS}
    ${GST_LIBRARIES}
    ${K4A_LIBRARY}/bin/libk4a.so
    )

# link libraries
if(EXECUTE_TARGET)
    add_custom_command(TARGET SSKinect
        POST_BUILD
        COMMAND ./build/bin/SSKinect --logtostderr=1
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
endif()
