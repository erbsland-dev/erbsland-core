# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_git_version(
        TARGET erbsland-core
        PROJECT_ROOT "${PROJECT_SOURCE_DIR}"
        TEMPLATE "${CMAKE_CURRENT_SOURCE_DIR}/LibraryVersion.in.cpp"
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/LibraryVersion.cpp"
        VARIABLE_PREFIX ERBSLAND_CORE_VERSION
)

target_include_directories(erbsland-core PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_sources(erbsland-core PRIVATE
        LibraryVersion.hpp
)

if (WIN32)
target_sources(erbsland-core PRIVATE
        WindowsApi.hpp
)
endif ()
