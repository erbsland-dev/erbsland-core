# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-file-search-support STATIC)
target_link_libraries(erbsland-core-file-search-support PUBLIC erbsland::core)
target_compile_features(erbsland-core-file-search-support PUBLIC cxx_std_20)
target_include_directories(erbsland-core-file-search-support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
erbsland_set_required_compiler_options(erbsland-core-file-search-support)
erbsland_enable_debug_warnings(erbsland-core-file-search-support)

erbsland_core_add_demo(file_search)
target_link_libraries(file_search PRIVATE erbsland-core-file-search-support)
add_subdirectory(src)

if(ERBSLAND_CORE_ENABLE_DEMOS AND ERBSLAND_CORE_ENABLE_TESTS)
    if(NOT TARGET erbsland-unittest)
        add_subdirectory("${PROJECT_SOURCE_DIR}/test/erbsland-unittest"
                "${CMAKE_BINARY_DIR}/test/erbsland-unittest" EXCLUDE_FROM_ALL)
    endif()
    add_subdirectory(test)
endif()
