# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
project(erbsland-core-resource-installed-package LANGUAGES CXX)

find_package(erbsland-core CONFIG REQUIRED)

add_executable(installed-resource-consumer main.cpp)
erbsland_core_add_resources(
        TARGET installed-resource-consumer
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "installed"
)
