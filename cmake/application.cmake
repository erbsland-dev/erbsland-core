# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

function(erbsland_core_setup_application)
    set(options "")
    set(one_value_args TARGET)
    set(multi_value_args "")
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments for erbsland_core_setup_application: ${ARGS_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARGS_TARGET)
        message(FATAL_ERROR "erbsland_core_setup_application requires TARGET.")
    endif()
    if(NOT TARGET "${ARGS_TARGET}")
        message(FATAL_ERROR "erbsland_core_setup_application target does not exist: ${ARGS_TARGET}")
    endif()
    get_target_property(target_type "${ARGS_TARGET}" TYPE)
    if(NOT target_type STREQUAL "EXECUTABLE")
        message(FATAL_ERROR "erbsland_core_setup_application supports executable targets only: ${ARGS_TARGET}")
    endif()
    get_target_property(is_configured "${ARGS_TARGET}" ERBSLAND_CORE_APPLICATION_CONFIGURED)
    if(is_configured)
        return()
    endif()

    if(TARGET erbsland::core)
        set(core_target erbsland::core)
    elseif(TARGET ErbslandDEV::erbsland-core)
        set(core_target ErbslandDEV::erbsland-core)
    else()
        message(FATAL_ERROR "The Erbsland Core target is unavailable.")
    endif()
    target_link_libraries("${ARGS_TARGET}" PRIVATE "${core_target}")
    target_compile_features("${ARGS_TARGET}" PRIVATE cxx_std_20)
    set_target_properties("${ARGS_TARGET}" PROPERTIES
            CXX_SCAN_FOR_MODULES OFF
            ERBSLAND_CORE_APPLICATION_CONFIGURED TRUE
    )
    if(MSVC)
        target_compile_options("${ARGS_TARGET}" PRIVATE /utf-8)
    endif()
endfunction()
