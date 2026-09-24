# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/application.cmake")

set(ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE "" CACHE FILEPATH
        "Native host erbsland-core-resource-compiler used while cross-compiling")

# Test whether a path has one of the requested suffixes.
#
# Parameters:
#   out_variable - Name of the variable that receives TRUE or FALSE in the caller's scope.
#   path - Path to test.
#   ARGN - Suffixes accepted for the path. With no suffixes, every path is accepted.
function(_erbsland_core_resource_has_suffix out_variable path)
    if(NOT ARGN)
        set("${out_variable}" TRUE PARENT_SCOPE)
        return()
    endif()
    string(LENGTH "${path}" path_length)
    foreach(suffix IN LISTS ARGN)
        string(LENGTH "${suffix}" suffix_length)
        if(suffix_length LESS_EQUAL path_length)
            math(EXPR suffix_start "${path_length} - ${suffix_length}")
            string(SUBSTRING "${path}" ${suffix_start} ${suffix_length} ending)
            if(ending STREQUAL suffix)
                set("${out_variable}" TRUE PARENT_SCOPE)
                return()
            endif()
        endif()
    endforeach()
    set("${out_variable}" FALSE PARENT_SCOPE)
endfunction()

# Select the resource-compiler command and its build dependency.
#
# Parameters:
#   out_command - Name of the variable that receives the command in the caller's scope.
#   out_dependency - Name of the variable that receives the file or target dependency in the caller's scope.
#
# A configured ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE takes precedence. Native builds otherwise use the in-tree or
# imported resource-compiler target. Cross-compiling requires an explicit native host executable.
function(_erbsland_core_resource_compiler out_command out_dependency)
    if(ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE)
        get_filename_component(compiler_path "${ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE}" ABSOLUTE)
        if(NOT EXISTS "${compiler_path}")
            message(FATAL_ERROR
                    "ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE does not exist: ${compiler_path}")
        endif()
        set("${out_command}" "${compiler_path}" PARENT_SCOPE)
        set("${out_dependency}" "${compiler_path}" PARENT_SCOPE)
        return()
    endif()
    if(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR
                "Cross-compiling resources requires ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE to name a native host tool.")
    endif()
    if(TARGET erbsland-core-resource-compiler)
        set(compiler_target erbsland-core-resource-compiler)
    elseif(TARGET ErbslandDEV::erbsland-core-resource-compiler)
        set(compiler_target ErbslandDEV::erbsland-core-resource-compiler)
    else()
        message(FATAL_ERROR "The Erbsland Core resource compiler target is unavailable.")
    endif()
    set("${out_command}" "$<TARGET_FILE:${compiler_target}>" PARENT_SCOPE)
    set("${out_dependency}" "${compiler_target}" PARENT_SCOPE)
endfunction()

# Compile files from a directory into resources linked with a target.
#
# Usage:
#   erbsland_core_add_resources(
#       TARGET <target>
#       DIRECTORY <directory>
#       IDENTIFIER <identifier>
#       [DEPENDENCY <target>]
#       [SUFFIXES <suffix>...]
#       [RECURSIVE]
#       [NO_HASH]
#       [NO_COMPRESSION]
#   )
#
# Arguments:
#   TARGET <target> - Existing executable or static-library target that receives the compiled resources.
#   DIRECTORY <directory> - Resource directory, relative to the current source directory or absolute.
#   IDENTIFIER <identifier> - Portable ASCII namespace stored with each resource.
#   DEPENDENCY <target> - Optional target that must be built before the generated resource library.
#   SUFFIXES <suffix>... - Optional filename suffix filter. With no suffixes, every regular file is selected.
#   RECURSIVE - Include files in subdirectories instead of only the top level.
#   NO_HASH - Omit the SHA3-256 integrity digest from generated resource metadata.
#   NO_COMPRESSION - Store original bytes without testing LZ4 compression.
#
# The function rejects symbolic links, resources larger than 1 MiB, empty selections, and duplicate resource keys. It
# creates generated translation units in the current binary directory and links them through a whole-archive helper so
# registration objects are retained by the linker. Multiple calls for the same target append resources to that target.
function(erbsland_core_add_resources)
    set(options RECURSIVE NO_HASH NO_COMPRESSION)
    set(one_value_args TARGET DIRECTORY IDENTIFIER DEPENDENCY)
    set(multi_value_args SUFFIXES)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if(ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments for erbsland_core_add_resources: ${ARGS_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARGS_TARGET OR NOT ARGS_DIRECTORY OR NOT ARGS_IDENTIFIER)
        message(FATAL_ERROR "erbsland_core_add_resources requires TARGET, DIRECTORY, and IDENTIFIER.")
    endif()
    if(NOT ARGS_IDENTIFIER MATCHES "^[A-Za-z0-9][A-Za-z0-9_.-]*$")
        message(FATAL_ERROR "Resource identifiers must be non-empty portable ASCII tokens: ${ARGS_IDENTIFIER}")
    endif()
    if(NOT TARGET "${ARGS_TARGET}")
        message(FATAL_ERROR "erbsland_core_add_resources target does not exist: ${ARGS_TARGET}")
    endif()
    get_target_property(target_type "${ARGS_TARGET}" TYPE)
    if(target_type STREQUAL "EXECUTABLE")
        set(setup_application_after_resource TRUE)
    elseif(target_type STREQUAL "STATIC_LIBRARY")
        erbsland_core_setup_static_library(TARGET "${ARGS_TARGET}")
    else()
        message(FATAL_ERROR
                "erbsland_core_add_resources supports executable and static library targets only: ${ARGS_TARGET}")
    endif()
    get_target_property(resource_target "${ARGS_TARGET}" ERBSLAND_CORE_RESOURCE_TARGET)
    if(NOT resource_target OR resource_target STREQUAL "resource_target-NOTFOUND")
        set(resource_target "${ARGS_TARGET}-erbsland-resources")
        set(resource_link_target "${resource_target}-link")
        if(TARGET "${resource_target}")
            message(FATAL_ERROR "Compiled-resource target already exists: ${resource_target}")
        endif()
        if(TARGET "${resource_link_target}")
            message(FATAL_ERROR "Compiled-resource link target already exists: ${resource_link_target}")
        endif()
        add_library("${resource_target}" STATIC)
        add_library("${resource_link_target}" INTERFACE)
        if(TARGET erbsland::core)
            set(core_target erbsland::core)
        else()
            set(core_target ErbslandDEV::erbsland-core)
        endif()
        target_link_libraries("${resource_target}" PRIVATE "${core_target}")
        target_compile_features("${resource_target}" PRIVATE cxx_std_20)
        set_target_properties("${resource_target}" PROPERTIES CXX_SCAN_FOR_MODULES OFF)
        target_link_libraries("${resource_link_target}" INTERFACE "$<LINK_LIBRARY:WHOLE_ARCHIVE,${resource_target}>")
        target_link_libraries("${ARGS_TARGET}" PRIVATE "${resource_link_target}")
        set_target_properties("${ARGS_TARGET}" PROPERTIES
                ERBSLAND_CORE_RESOURCE_TARGET "${resource_target}"
        )
    endif()
    if(ARGS_DEPENDENCY)
        if(NOT TARGET "${ARGS_DEPENDENCY}")
            message(FATAL_ERROR
                    "erbsland_core_add_resources dependency target does not exist: ${ARGS_DEPENDENCY}")
        endif()
        add_dependencies("${resource_target}" "${ARGS_DEPENDENCY}")
    endif()
    if(setup_application_after_resource)
        erbsland_core_setup_application(TARGET "${ARGS_TARGET}")
    endif()
    set(output_directory "${CMAKE_CURRENT_BINARY_DIR}/erbsland-resources/${ARGS_TARGET}")
    if(target_type STREQUAL "STATIC_LIBRARY")
        get_target_property(target_sources "${ARGS_TARGET}" SOURCES)
        if(NOT target_sources OR target_sources STREQUAL "target_sources-NOTFOUND")
            set(placeholder_source "${output_directory}/static-library-placeholder.cpp")
            file(GENERATE OUTPUT "${placeholder_source}"
                    CONTENT "// Generated by erbsland_core_add_resources().\n")
            set_source_files_properties("${placeholder_source}" PROPERTIES
                    GENERATED TRUE
                    SKIP_PRECOMPILE_HEADERS TRUE
                    SKIP_UNITY_BUILD_INCLUSION TRUE
            )
            target_sources("${ARGS_TARGET}" PRIVATE "${placeholder_source}")
        endif()
    endif()

    get_filename_component(resource_directory "${ARGS_DIRECTORY}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT IS_DIRECTORY "${resource_directory}")
        message(FATAL_ERROR "Resource directory does not exist: ${resource_directory}")
    endif()
    if(ARGS_RECURSIVE)
        file(GLOB_RECURSE selected_files CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE
                RELATIVE "${resource_directory}" "${resource_directory}/*")
    else()
        file(GLOB selected_files CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE
                RELATIVE "${resource_directory}" "${resource_directory}/*")
    endif()
    list(SORT selected_files)

    set(filtered_files "")
    foreach(relative_path IN LISTS selected_files)
        set(absolute_path "${resource_directory}/${relative_path}")
        if(IS_SYMLINK "${absolute_path}")
            message(FATAL_ERROR "Compiled resources cannot be symbolic links: ${absolute_path}")
        endif()
        _erbsland_core_resource_has_suffix(has_suffix "${relative_path}" ${ARGS_SUFFIXES})
        if(NOT has_suffix)
            continue()
        endif()
        file(SIZE "${absolute_path}" resource_size)
        if(resource_size GREATER 1048576)
            message(FATAL_ERROR "Compiled resource exceeds the 1 MiB limit: ${absolute_path}")
        endif()
        list(APPEND filtered_files "${relative_path}")
    endforeach()
    if(NOT filtered_files)
        message(FATAL_ERROR "The compiled-resource selection is empty for identifier '${ARGS_IDENTIFIER}'.")
    endif()

    _erbsland_core_resource_compiler(compiler_command compiler_dependency)
    set(generated_sources "")
    get_target_property(existing_keys "${ARGS_TARGET}" ERBSLAND_CORE_RESOURCE_KEYS)
    if(NOT existing_keys OR existing_keys STREQUAL "existing_keys-NOTFOUND")
        set(existing_keys "")
    endif()
    foreach(relative_path IN LISTS filtered_files)
        string(REPLACE "\\" "/" normalized_path "${relative_path}")
        string(SHA256 key_hash "${ARGS_IDENTIFIER}\n${normalized_path}")
        if(key_hash IN_LIST existing_keys)
            message(FATAL_ERROR
                    "Duplicate compiled-resource key for target ${ARGS_TARGET}: (${ARGS_IDENTIFIER}, ${normalized_path})")
        endif()
        list(APPEND existing_keys "${key_hash}")
        string(SUBSTRING "${key_hash}" 0 24 symbol)

        set(data_output "${output_directory}/${symbol}-data.cpp")
        set(descriptor_output "${output_directory}/${symbol}-descriptor.cpp")
        set(optional_arguments "")
        if(ARGS_NO_HASH)
            list(APPEND optional_arguments --no-hash)
        endif()
        if(ARGS_NO_COMPRESSION)
            list(APPEND optional_arguments --no-compression)
        endif()
        add_custom_command(
                OUTPUT "${data_output}" "${descriptor_output}"
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_directory}"
                COMMAND "${compiler_command}"
                        --protocol 3
                        --input "${resource_directory}/${relative_path}"
                        --data-output "${data_output}"
                        --descriptor-output "${descriptor_output}"
                        --identifier "${ARGS_IDENTIFIER}"
                        --resource-path "${normalized_path}"
                        --symbol "${symbol}"
                        ${optional_arguments}
                DEPENDS "${resource_directory}/${relative_path}" "${compiler_dependency}"
                COMMENT "Compiling resource ${ARGS_IDENTIFIER}:${normalized_path}"
                VERBATIM
        )
        set_source_files_properties("${data_output}" "${descriptor_output}" PROPERTIES
                GENERATED TRUE
                SKIP_PRECOMPILE_HEADERS TRUE
                SKIP_UNITY_BUILD_INCLUSION TRUE
        )
        list(APPEND generated_sources "${data_output}" "${descriptor_output}")
    endforeach()
    set_target_properties("${ARGS_TARGET}" PROPERTIES ERBSLAND_CORE_RESOURCE_KEYS "${existing_keys}")
    target_sources("${resource_target}" PRIVATE ${generated_sources})
endfunction()
