# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-pi-calculator-support STATIC)
target_link_libraries(erbsland-core-pi-calculator-support PUBLIC erbsland::core)
target_compile_features(erbsland-core-pi-calculator-support PUBLIC cxx_std_20)
target_include_directories(erbsland-core-pi-calculator-support PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
erbsland_set_required_compiler_options(erbsland-core-pi-calculator-support)
erbsland_enable_debug_warnings(erbsland-core-pi-calculator-support)

erbsland_core_add_demo(pi_calculator)
target_link_libraries(pi_calculator PRIVATE erbsland-core-pi-calculator-support)
add_subdirectory(src)

if(ERBSLAND_CORE_ENABLE_DEMOS AND ERBSLAND_CORE_ENABLE_TESTS)
    if(NOT TARGET erbsland-unittest)
        add_subdirectory("${PROJECT_SOURCE_DIR}/test/erbsland-unittest"
                "${CMAKE_BINARY_DIR}/test/erbsland-unittest" EXCLUDE_FROM_ALL)
    endif()
    add_subdirectory(test)
endif()
