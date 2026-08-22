# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/application.cmake")

set(ERBSLAND_CORE_RESOURCE_COMPILER_EXECUTABLE "" CACHE FILEPATH
        "Native host erbsland-core-resource-compiler used while cross-compiling")

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

function(erbsland_core_add_resources)
    set(options RECURSIVE NO_HASH NO_COMPRESSION)
    set(one_value_args TARGET DIRECTORY IDENTIFIER)
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
    erbsland_core_setup_application(TARGET "${ARGS_TARGET}")

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

        set(output_directory "${CMAKE_CURRENT_BINARY_DIR}/erbsland-resources/${ARGS_TARGET}")
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
                        --protocol 2
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
    target_sources("${ARGS_TARGET}" PRIVATE ${generated_sources})
endfunction()
