# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

set(ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE "" CACHE FILEPATH
        "Native host erbsland-core-display-text-compiler used while cross-compiling")

# Compile an ELCL catalog into build-directory C++ sources.
# Usage: erbsland_core_display_text(TARGET <target> FILE <elcl>
#                                 [HEADER <hpp>] [SOURCE <cpp>] [NAMESPACE <namespace>] [CLASS <identifier>])
# FILE is relative to the source directory; outputs are relative to the binary directory.
# Default outputs use the input stem, which also supplies the default CLASS.
function(erbsland_core_display_text)
    cmake_parse_arguments(ARGS "" "TARGET;FILE;HEADER;SOURCE;NAMESPACE;CLASS" "" ${ARGN})
    if(ARGS_UNPARSED_ARGUMENTS OR ARGS_KEYWORDS_MISSING_VALUES OR NOT ARGS_TARGET OR NOT ARGS_FILE)
        message(FATAL_ERROR "erbsland_core_display_text requires TARGET and FILE, with valid optional arguments.")
    endif()
    if(NOT TARGET "${ARGS_TARGET}")
        message(FATAL_ERROR "Display-text target does not exist: ${ARGS_TARGET}")
    endif()
    get_filename_component(input "${ARGS_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT EXISTS "${input}")
        message(FATAL_ERROR "Display-text source does not exist: ${input}")
    endif()
    get_filename_component(stem "${input}" NAME_WLE)
    if(NOT ARGS_HEADER)
        set(ARGS_HEADER "${CMAKE_CURRENT_BINARY_DIR}/${stem}.hpp")
    endif()
    if(NOT ARGS_SOURCE)
        set(ARGS_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${stem}.cpp")
    endif()
    if(NOT ARGS_CLASS)
        set(ARGS_CLASS "${stem}")
    endif()
    if(NOT ARGS_CLASS MATCHES "^[A-Za-z][A-Za-z0-9_]*$"
            OR (ARGS_NAMESPACE AND NOT ARGS_NAMESPACE MATCHES "^[A-Za-z][A-Za-z0-9_]*(::[A-Za-z][A-Za-z0-9_]*)*$"))
        message(FATAL_ERROR "Display-text CLASS and NAMESPACE must use C++ identifiers.")
    endif()
    get_filename_component(header "${ARGS_HEADER}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    get_filename_component(source "${ARGS_SOURCE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    if(input STREQUAL header OR input STREQUAL source OR header STREQUAL source)
        message(FATAL_ERROR "Display-text input, header, and source must be distinct files.")
    endif()
    # There must be only one writer for each output, even across targets.
    foreach(output IN ITEMS "${header}" "${source}")
        cmake_path(IS_PREFIX CMAKE_BINARY_DIR "${output}" NORMALIZE in_build_tree)
        if(NOT in_build_tree)
            message(FATAL_ERROR "Display-text outputs must remain in the build tree: ${output}")
        endif()
        string(SHA256 output_key "${output}")
        get_property(existing_writer GLOBAL PROPERTY "ERBSLAND_CORE_DISPLAY_TEXT_OUTPUT_${output_key}")
        if(existing_writer)
            message(FATAL_ERROR "Duplicate display-text output: ${output}")
        endif()
        set_property(GLOBAL PROPERTY "ERBSLAND_CORE_DISPLAY_TEXT_OUTPUT_${output_key}" TRUE)
    endforeach()
    set(compiler_dependency "")
    if(ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE)
        get_filename_component(compiler "${ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE}" ABSOLUTE)
        if(NOT EXISTS "${compiler}")
            message(FATAL_ERROR "ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE does not exist: ${compiler}")
        endif()
    elseif(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Cross-compiling display texts requires ERBSLAND_CORE_DISPLAY_TEXT_COMPILER_EXECUTABLE to name a native host tool.")
    elseif(TARGET erbsland::core-display-text-compiler)
        set(compiler "$<TARGET_FILE:erbsland::core-display-text-compiler>")
        set(compiler_dependency erbsland::core-display-text-compiler)
    elseif(TARGET ErbslandDEV::erbsland-core-display-text-compiler)
        set(compiler "$<TARGET_FILE:ErbslandDEV::erbsland-core-display-text-compiler>")
        set(compiler_dependency ErbslandDEV::erbsland-core-display-text-compiler)
    else()
        message(FATAL_ERROR "The Erbsland Core display-text compiler is unavailable.")
    endif()
    set(optional_arguments "")
    if(ARGS_NAMESPACE)
        list(APPEND optional_arguments --namespace "${ARGS_NAMESPACE}")
    endif()
    # A generated dependency also invalidates Makefile builds when generation arguments change.
    string(SHA256 catalog_key "${header}\n${source}")
    set(arguments_file "${CMAKE_CURRENT_BINARY_DIR}/display-text/${catalog_key}.arguments")
    file(GENERATE OUTPUT "${arguments_file}" CONTENT
            "${input}\n${header}\n${source}\n${ARGS_CLASS}\n${ARGS_NAMESPACE}\n")
    add_custom_command(
            OUTPUT "${header}" "${source}"
            COMMAND "${compiler}" --input "${input}" --header-output "${header}" --source-output "${source}"
                    --class "${ARGS_CLASS}" ${optional_arguments}
            DEPENDS "${input}" "${compiler}" ${compiler_dependency} "${arguments_file}"
            COMMENT "Compiling display texts ${ARGS_FILE}"
            VERBATIM
    )
    get_filename_component(header_directory "${header}" DIRECTORY)
    target_include_directories("${ARGS_TARGET}" PRIVATE "${header_directory}")
    set_source_files_properties("${header}" "${source}" TARGET_DIRECTORY "${ARGS_TARGET}" PROPERTIES
            GENERATED TRUE SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_PRECOMPILE_HEADERS TRUE)
    target_sources("${ARGS_TARGET}" PRIVATE "${header}" "${source}")
endfunction()
