# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

# Configure the common Erbsland Core requirements on a target.
#
# Parameters:
#   target - Name of an existing CMake target to configure.
#   visibility - Link and compile-feature visibility (for example, PRIVATE or PUBLIC).
#   configured_property - Target property used to make the configuration idempotent.
#
# This internal helper links the available in-tree or installed Erbsland Core target, requires C++20, disables C++
# module scanning, and enables UTF-8 source handling with MSVC. If configured_property is already true, the function
# leaves the target unchanged.
function(_erbsland_core_setup_target target visibility configured_property)
    get_target_property(is_configured "${target}" "${configured_property}")
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
    target_link_libraries("${target}" "${visibility}" "${core_target}")
    target_compile_features("${target}" "${visibility}" cxx_std_20)
    set_target_properties("${target}" PROPERTIES
            CXX_SCAN_FOR_MODULES OFF
            "${configured_property}" TRUE
    )
    if(MSVC)
        target_compile_options("${target}" "${visibility}" /utf-8)
    endif()
endfunction()

# Configure an executable target as an Erbsland Core application.
#
# Usage:
#   erbsland_core_setup_application(TARGET <target>)
#
# Arguments:
#   TARGET <target> - Name of an existing executable target. Erbsland Core is linked privately.
#
# The operation is idempotent. Unsupported target types, missing targets, and unexpected arguments are reported as
# configuration errors.
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
    _erbsland_core_setup_target("${ARGS_TARGET}" PRIVATE ERBSLAND_CORE_APPLICATION_CONFIGURED)
endfunction()

# Configure a static-library target that exposes Erbsland Core to its consumers.
#
# Usage:
#   erbsland_core_setup_static_library(TARGET <target>)
#
# Arguments:
#   TARGET <target> - Name of an existing static-library target. Erbsland Core is linked publicly.
#
# The operation is idempotent. Unsupported target types, missing targets, and unexpected arguments are reported as
# configuration errors.
function(erbsland_core_setup_static_library)
    set(options "")
    set(one_value_args TARGET)
    set(multi_value_args "")
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments for erbsland_core_setup_static_library: ${ARGS_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARGS_TARGET)
        message(FATAL_ERROR "erbsland_core_setup_static_library requires TARGET.")
    endif()
    if(NOT TARGET "${ARGS_TARGET}")
        message(FATAL_ERROR "erbsland_core_setup_static_library target does not exist: ${ARGS_TARGET}")
    endif()
    get_target_property(target_type "${ARGS_TARGET}" TYPE)
    if(NOT target_type STREQUAL "STATIC_LIBRARY")
        message(FATAL_ERROR "erbsland_core_setup_static_library supports static library targets only: ${ARGS_TARGET}")
    endif()
    _erbsland_core_setup_target("${ARGS_TARGET}" PUBLIC ERBSLAND_CORE_STATIC_LIBRARY_CONFIGURED)
endfunction()
