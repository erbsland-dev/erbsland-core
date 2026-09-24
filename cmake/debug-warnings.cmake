# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.23)
include_guard()

# Enable strict compiler diagnostics for Debug builds of a target.
#
# Parameters:
#   target - Name of an existing CMake target.
#
# Warnings are treated as errors. MSVC uses its high warning level; GCC and Clang use the shared warning set, and
# Clang additionally retains frame pointers to improve debugging and profiling.
function(erbsland_enable_debug_warnings target)
    if(MSVC)
        target_compile_options(${target} BEFORE PRIVATE
                $<$<CONFIG:Debug>:/W4>
                $<$<CONFIG:Debug>:/WX>
        )
    else()
        target_compile_options(${target} BEFORE PRIVATE
                $<$<CONFIG:Debug>:-Wall>
                $<$<CONFIG:Debug>:-Wextra>
                $<$<CONFIG:Debug>:-Werror>
                $<$<CONFIG:Debug>:-Wconversion>
        )
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
                    $<$<CONFIG:Debug>:-mno-omit-leaf-frame-pointer>
            )
        endif()
    endif()
endfunction()
