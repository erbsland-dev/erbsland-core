# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_library(erbsland-core-demo-common STATIC)
add_subdirectory(src)

add_library(erbsland-core::demo-common ALIAS erbsland-core-demo-common)
target_compile_features(erbsland-core-demo-common PUBLIC cxx_std_20)
target_link_libraries(erbsland-core-demo-common PUBLIC erbsland::core)
target_include_directories(erbsland-core-demo-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
erbsland_set_required_compiler_options(erbsland-core-demo-common)
erbsland_enable_debug_warnings(erbsland-core-demo-common)
set_target_properties(erbsland-core-demo-common PROPERTIES
        FOLDER "Demos"
)

add_custom_target(erbsland-core-all-demos)
set_target_properties(erbsland-core-all-demos PROPERTIES
        FOLDER "Demos"
)

function(erbsland_core_add_demo target_name)
    add_executable(${target_name} ${ARGN})
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_link_libraries(${target_name}
            PRIVATE
            erbsland-core-demo-common
    )
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)
    erbsland_set_required_compiler_options(${target_name})
    erbsland_enable_debug_warnings(${target_name})
    set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demo-apps"
            FOLDER "Demos"
    )
    add_dependencies(erbsland-core-all-demos ${target_name})
endfunction()
