# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.23)
include_guard()

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
