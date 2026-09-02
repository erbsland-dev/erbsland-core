# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
project(erbsland-core-resource-cmake-error LANGUAGES CXX)

if(NOT DEFINED ERBSLAND_CORE_SOURCE OR NOT DEFINED RESOURCE_ERROR_CASE)
    message(FATAL_ERROR "This fixture requires ERBSLAND_CORE_SOURCE and RESOURCE_ERROR_CASE.")
endif()
set(ERBSLAND_CORE_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_ENABLE_DEMOS OFF CACHE BOOL "" FORCE)
set(ERBSLAND_CORE_DEVELOPER_BUILD OFF CACHE BOOL "" FORCE)
add_subdirectory("${ERBSLAND_CORE_SOURCE}" erbsland-core EXCLUDE_FROM_ALL)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/data/value.txt" "fixture")

if(RESOURCE_ERROR_CASE STREQUAL "invalid-target")
    add_library(fixture STATIC main.cpp)
    erbsland_core_setup_application(TARGET fixture)
elseif(RESOURCE_ERROR_CASE STREQUAL "invalid-static-library-target")
    add_executable(fixture main.cpp)
    erbsland_core_setup_static_library(TARGET fixture)
elseif(RESOURCE_ERROR_CASE STREQUAL "invalid-resource-target")
    add_library(fixture SHARED main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data")
elseif(RESOURCE_ERROR_CASE STREQUAL "export-static-library")
    add_library(fixture STATIC main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data")
    install(TARGETS fixture erbsland-core EXPORT fixture-targets)
    install(EXPORT fixture-targets FILE fixture-targets.cmake DESTINATION lib/cmake/fixture)
elseif(RESOURCE_ERROR_CASE STREQUAL "duplicate")
    add_executable(fixture main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data")
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data")
elseif(RESOURCE_ERROR_CASE STREQUAL "empty")
    add_executable(fixture main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data" SUFFIXES ".json")
elseif(RESOURCE_ERROR_CASE STREQUAL "oversized")
    string(REPEAT "x" 1048577 oversized_data)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/data/oversized.bin" "${oversized_data}")
    add_executable(fixture main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "data")
elseif(RESOURCE_ERROR_CASE STREQUAL "identifier")
    add_executable(fixture main.cpp)
    erbsland_core_add_resources(
            TARGET fixture DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data" IDENTIFIER "not portable")
else()
    message(FATAL_ERROR "Unknown RESOURCE_ERROR_CASE: ${RESOURCE_ERROR_CASE}")
endif()
