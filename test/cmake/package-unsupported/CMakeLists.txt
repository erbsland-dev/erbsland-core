# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
project(package-unsupported VERSION 1.0.0 LANGUAGES CXX)

if(NOT PACKAGE_SOURCE_DIRECTORY)
    message(FATAL_ERROR "PACKAGE_SOURCE_DIRECTORY is required.")
endif()

include("${PACKAGE_SOURCE_DIRECTORY}/cmake/package.cmake")

add_executable(package_unsupported main.cpp)
install(TARGETS package_unsupported RUNTIME DESTINATION bin)

# Exercise the same branch on Windows and macOS hosts without changing their toolchain.
set(WIN32 FALSE)
set(APPLE FALSE)
if(PACKAGE_INVALID_TARGET)
    add_executable(package-unsupported main.cpp)
    erbsland_core_package(TARGET package-unsupported NAME unsupported)
elseif(PACKAGE_INVALID_NAME)
    erbsland_core_package(TARGET package_unsupported NAME invalid-name)
else()
    erbsland_core_package(TARGET package_unsupported NAME unsupported)
endif()
