# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

function(erbsland_core_normalize_uint16 out_variable value)
    string(REGEX REPLACE "^0+" "" normalized_value "${value}")
    if (normalized_value STREQUAL "")
        set(normalized_value "0")
    endif ()

    string(LENGTH "${normalized_value}" value_length)
    if (value_length GREATER 5)
        set(${out_variable} "" PARENT_SCOPE)
        return()
    endif ()
    if (value_length EQUAL 5 AND normalized_value STRGREATER "65535")
        set(${out_variable} "" PARENT_SCOPE)
        return()
    endif ()
    set(${out_variable} "${normalized_value}" PARENT_SCOPE)
endfunction()

function(erbsland_core_generate_version_source)
    set(ERBSLAND_CORE_VERSION_MAJOR "0")
    set(ERBSLAND_CORE_VERSION_MINOR "0")
    set(ERBSLAND_CORE_VERSION_REVISION "0")
    set(ERBSLAND_CORE_VERSION_BUILD "0")
    set(ERBSLAND_CORE_VERSION_TEXT "0.0.0.0")

    find_package(Git QUIET)
    if (GIT_FOUND AND EXISTS "${ERBSLAND_CORE_SOURCE_DIR}/.git")
        execute_process(
                COMMAND "${GIT_EXECUTABLE}" describe --tags --long --dirty --always
                WORKING_DIRECTORY "${ERBSLAND_CORE_SOURCE_DIR}"
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_describe
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (git_result EQUAL 0)
            string(REGEX MATCH
                    "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)-g[0-9A-Fa-f]+(-dirty)?$"
                    version_match
                    "${git_describe}"
            )
            if (version_match)
                erbsland_core_normalize_uint16(version_major "${CMAKE_MATCH_1}")
                erbsland_core_normalize_uint16(version_minor "${CMAKE_MATCH_2}")
                erbsland_core_normalize_uint16(version_revision "${CMAKE_MATCH_3}")
                erbsland_core_normalize_uint16(version_build "${CMAKE_MATCH_4}")
                if (NOT version_major STREQUAL "" AND NOT version_minor STREQUAL "" AND
                        NOT version_revision STREQUAL "" AND NOT version_build STREQUAL "")
                    set(ERBSLAND_CORE_VERSION_MAJOR "${version_major}")
                    set(ERBSLAND_CORE_VERSION_MINOR "${version_minor}")
                    set(ERBSLAND_CORE_VERSION_REVISION "${version_revision}")
                    set(ERBSLAND_CORE_VERSION_BUILD "${version_build}")
                    set(ERBSLAND_CORE_VERSION_TEXT "${git_describe}")
                else ()
                    message(WARNING
                            "Could not use Git version '${git_describe}' because a version part exceeds 65535. "
                            "Falling back to 0.0.0.0."
                    )
                endif ()
            endif ()
        endif ()
    endif ()

    get_filename_component(output_directory "${ERBSLAND_CORE_OUTPUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${output_directory}")
    configure_file("${ERBSLAND_CORE_TEMPLATE}" "${ERBSLAND_CORE_OUTPUT}" @ONLY)
endfunction()

function(erbsland_core_add_git_version_source target template output)
    add_custom_target(erbsland-core-version
            BYPRODUCTS "${output}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DERBSLAND_CORE_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
                    "-DERBSLAND_CORE_TEMPLATE=${template}"
                    "-DERBSLAND_CORE_OUTPUT=${output}"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            DEPENDS
                    "${template}"
                    "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            VERBATIM
    )
    set_source_files_properties("${output}" PROPERTIES GENERATED TRUE)
    target_sources(${target} PRIVATE "${output}")
    add_dependencies(${target} erbsland-core-version)
endfunction()

if (CMAKE_SCRIPT_MODE_FILE)
    erbsland_core_generate_version_source()
endif ()
