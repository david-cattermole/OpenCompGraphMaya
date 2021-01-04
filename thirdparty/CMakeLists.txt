# Build third party projects as part of OpenCompGraphMaya, but in
# their own build directories.

cmake_minimum_required(VERSION 3.0.0)

# Project configuration.
project(Project)

# Include the CMake "ExternalProject" tools, otherwise nothing will
# work.
include(ExternalProject)

# Configuration
set(THIRDPARTY_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")

# spdlog - Used for logging features in the C++ Maya plug-in.
ExternalProject_Add(spdlog
    PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
    GIT_TAG "v1.8.2"
    INSTALL_DIR ${THIRDPARTY_INSTALL_PREFIX}
    CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DCMAKE_INSTALL_PREFIX=${THIRDPARTY_INSTALL_PREFIX}
        -DSPDLOG_BUILD_SHARED=OFF
        -DSPDLOG_BUILD_EXAMPLE=OFF
)
