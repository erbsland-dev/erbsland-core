# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.23)
include_guard()

function(erbsland_set_required_compiler_options target)
    set_target_properties(${target} PROPERTIES LINKER_LANGUAGE CXX)
    target_compile_features(${target} PUBLIC cxx_std_20)

    if(MSVC)
        target_compile_options(${target} PUBLIC /utf-8)
        target_compile_options(${target} PRIVATE /MP)
    endif()

    if(ERBSLAND_CORE_DO_NOT_FLATTEN_NS)
        target_compile_definitions(${target} PUBLIC -DERBSLAND_CORE_DO_NOT_FLATTEN_NS=1)
    endif()
    if(${PROJECT_IS_TOP_LEVEL})
        target_compile_definitions(${target} PUBLIC $<$<CONFIG:Debug>:-DERBSLAND_CORE_DEVELOPER_BUILD=1>)
    endif()
endfunction()
