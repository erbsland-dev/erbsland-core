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

add_executable(installed-resource-consumer main.cpp)
target_link_libraries(installed-resource-consumer PRIVATE installed-resource-library)
