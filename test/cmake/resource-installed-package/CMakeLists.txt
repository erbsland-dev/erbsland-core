# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
project(erbsland-core-resource-installed-package LANGUAGES CXX)

find_package(erbsland-core CONFIG REQUIRED)

add_library(installed-resource-library STATIC)
erbsland_core_add_resources(
        TARGET installed-resource-library
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "installed"
)

add_executable(installed-resource-consumer)
target_link_libraries(installed-resource-consumer PRIVATE installed-resource-library)

# Exercise the same public namespace and include contract through the installed package.
add_library(installed-namespace-consumer OBJECT)
target_link_libraries(installed-namespace-consumer PRIVATE ErbslandDEV::erbsland-core)
add_library(installed-namespace-explicit-consumer OBJECT)
target_link_libraries(installed-namespace-explicit-consumer PRIVATE ErbslandDEV::erbsland-core)
target_compile_definitions(installed-namespace-explicit-consumer PRIVATE ERBSLAND_CORE_DO_NOT_FLATTEN_NS=1)

add_subdirectory(src)
add_subdirectory(../namespace-integration/src namespace-sources)
