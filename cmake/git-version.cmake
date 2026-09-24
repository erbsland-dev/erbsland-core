# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)
include_guard(GLOBAL)

# Normalize an unsigned 16-bit decimal value for generated version components.
#
# Parameters:
#   out_variable - Name of the variable that receives the result in the caller's scope.
#   value - Decimal text to normalize.
#
# Leading zeroes are removed. Zero is returned as "0"; a value greater than 65535 is reported as an empty result.
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

# Resolve a path against a base directory.
#
# Parameters:
#   out_variable - Name of the variable that receives the absolute path in the caller's scope.
#   path - Absolute path, or a relative path to resolve.
#   base_directory - Directory used to resolve a relative path.
function(erbsland_core_resolve_path out_variable path base_directory)
    if (IS_ABSOLUTE "${path}")
        set(resolved_path "${path}")
    else ()
        get_filename_component(resolved_path "${base_directory}/${path}" ABSOLUTE)
    endif ()
    set(${out_variable} "${resolved_path}" PARENT_SCOPE)
endfunction()

# Locate the Git directory associated with a source directory.
#
# Parameters:
#   out_variable - Name of the variable that receives the Git-directory path in the caller's scope.
#   source_directory - Working-tree directory whose .git entry is inspected.
#
# Both regular repositories with a .git directory and linked worktrees with a gitdir file are supported. The output is
# empty when no usable Git directory can be resolved.
function(erbsland_core_resolve_git_directory out_variable source_directory)
    set(git_entry "${source_directory}/.git")
    set(git_directory "")

    if (IS_DIRECTORY "${git_entry}")
        set(git_directory "${git_entry}")
    elseif (EXISTS "${git_entry}")
        file(READ "${git_entry}" git_entry_content)
        string(REGEX MATCH "^gitdir: ([^\r\n]+)" gitdir_match "${git_entry_content}")
        if (gitdir_match)
            erbsland_core_resolve_path(git_directory "${CMAKE_MATCH_1}" "${source_directory}")
        endif ()
    endif ()

    set(${out_variable} "${git_directory}" PARENT_SCOPE)
endfunction()

# Resolve the common Git directory shared by linked worktrees.
#
# Parameters:
#   out_variable - Name of the variable that receives the common Git-directory path in the caller's scope.
#   git_directory - Git directory to inspect for a commondir file.
#
# For a regular repository, or when commondir is unavailable, git_directory itself is returned.
function(erbsland_core_resolve_common_git_directory out_variable git_directory)
    set(common_git_directory "${git_directory}")
    set(common_dir_file "${git_directory}/commondir")
    if (EXISTS "${common_dir_file}")
        file(READ "${common_dir_file}" common_dir_content)
        string(STRIP "${common_dir_content}" common_dir_content)
        if (NOT common_dir_content STREQUAL "")
            erbsland_core_resolve_path(common_git_directory "${common_dir_content}" "${git_directory}")
        endif ()
    endif ()
    set(${out_variable} "${common_git_directory}" PARENT_SCOPE)
endfunction()

# Append a path to a dependency-list variable when the path exists.
#
# Parameters:
#   dependency_list_variable - Name of a list variable to update in the caller's scope.
#   path - File-system path to append when it exists.
function(erbsland_core_add_existing_dependency dependency_list_variable path)
    set(dependency_list ${${dependency_list_variable}})
    if (EXISTS "${path}")
        list(APPEND dependency_list "${path}")
    endif ()
    set(${dependency_list_variable} ${dependency_list} PARENT_SCOPE)
endfunction()

# Collect Git metadata files that can change the version reported for a source tree.
#
# Parameters:
#   out_variable - Name of the variable that receives the dependency list in the caller's scope.
#   source_directory - Working-tree directory to inspect.
#
# The result covers regular repositories and linked worktrees and contains only existing, unique paths. It can be empty
# when the directory is not a Git working tree or no relevant metadata exists.
function(erbsland_core_collect_git_dependencies out_variable source_directory)
    set(dependencies)
    set(git_entry "${source_directory}/.git")
    if (EXISTS "${git_entry}" AND NOT IS_DIRECTORY "${git_entry}")
        list(APPEND dependencies "${git_entry}")
    endif ()

    erbsland_core_resolve_git_directory(git_directory "${source_directory}")
    if (git_directory STREQUAL "")
        set(${out_variable} ${dependencies} PARENT_SCOPE)
        return()
    endif ()

    erbsland_core_resolve_common_git_directory(common_git_directory "${git_directory}")

    erbsland_core_add_existing_dependency(dependencies "${git_directory}/HEAD")
    erbsland_core_add_existing_dependency(dependencies "${git_directory}/logs/HEAD")
    erbsland_core_add_existing_dependency(dependencies "${common_git_directory}/packed-refs")
    erbsland_core_add_existing_dependency(dependencies "${common_git_directory}/logs/HEAD")

    if (dependencies)
        list(REMOVE_DUPLICATES dependencies)
    endif ()

    set(${out_variable} ${dependencies} PARENT_SCOPE)
endfunction()

# Generate a version source from Git metadata in CMake script mode.
#
# Inputs:
#   ERBSLAND_CORE_SOURCE_DIR - Source tree from which the Git description is read.
#   ERBSLAND_CORE_TEMPLATE - configure_file() input template.
#   ERBSLAND_CORE_OUTPUT - Destination path for the configured source.
#   ERBSLAND_GIT_VERSION_VARIABLE_PREFIX - Optional prefix for template variables; defaults to ERBSLAND_GIT_VERSION.
#
# The template receives <prefix>_MAJOR, _MINOR, _REVISION, _BUILD, and _TEXT. A matching Git description has the form
# v<major>.<minor>.<revision>-<build>-g<hash>, with each numeric part limited to uint16. Missing or unsuitable metadata
# produces the deterministic fallback version 0.0.0.0.
function(erbsland_core_generate_version_source)
    set(variable_prefix "${ERBSLAND_GIT_VERSION_VARIABLE_PREFIX}")
    if (variable_prefix STREQUAL "")
        set(variable_prefix "ERBSLAND_GIT_VERSION")
    endif ()

    set("${variable_prefix}_MAJOR" "0")
    set("${variable_prefix}_MINOR" "0")
    set("${variable_prefix}_REVISION" "0")
    set("${variable_prefix}_BUILD" "0")
    set("${variable_prefix}_TEXT" "0.0.0.0")

    find_package(Git QUIET)
    if (GIT_FOUND AND EXISTS "${ERBSLAND_CORE_SOURCE_DIR}/.git")
        execute_process(
                COMMAND "${GIT_EXECUTABLE}" describe --tags --long --always
                WORKING_DIRECTORY "${ERBSLAND_CORE_SOURCE_DIR}"
                RESULT_VARIABLE git_result
                OUTPUT_VARIABLE git_describe
                ERROR_QUIET
                OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (git_result EQUAL 0)
            string(REGEX MATCH
                    "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)-g[0-9A-Fa-f]+$"
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
                    set("${variable_prefix}_MAJOR" "${version_major}")
                    set("${variable_prefix}_MINOR" "${version_minor}")
                    set("${variable_prefix}_REVISION" "${version_revision}")
                    set("${variable_prefix}_BUILD" "${version_build}")
                    set("${variable_prefix}_TEXT" "${git_describe}")
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

# Add a generated Git-version source to a target.
#
# Usage:
#   erbsland_core_add_git_version(
#       TARGET <target>
#       PROJECT_ROOT <directory>
#       TEMPLATE <file>
#       [OUTPUT <file>]
#       [VARIABLE_PREFIX <prefix>]
#   )
#
# Arguments:
#   TARGET <target> - Existing target that receives the generated source.
#   PROJECT_ROOT <directory> - Git working tree, relative to the current source directory or absolute.
#   TEMPLATE <file> - configure_file() template, relative to the current source directory or absolute.
#   OUTPUT <file> - Optional output, relative to the current binary directory or absolute. By default, a target-specific
#                   directory and the template name without its .in component are used.
#   VARIABLE_PREFIX <prefix> - Optional CMake identifier prefix for version variables. The default is
#                              ERBSLAND_GIT_VERSION.
#
# The generated source is refreshed when the template, this module, or relevant Git metadata changes. Generation runs
# through this module in CMake script mode and is added as a source and dependency of the target.
function(erbsland_core_add_git_version)
    set(options)
    set(one_value_args TARGET PROJECT_ROOT TEMPLATE OUTPUT VARIABLE_PREFIX)
    set(multi_value_args)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (ARGS_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments for erbsland_core_add_git_version: ${ARGS_UNPARSED_ARGUMENTS}")
    endif ()
    if (ARGS_TARGET STREQUAL "")
        message(FATAL_ERROR "erbsland_core_add_git_version requires TARGET.")
    endif ()
    if (NOT TARGET "${ARGS_TARGET}")
        message(FATAL_ERROR "erbsland_core_add_git_version TARGET does not exist: ${ARGS_TARGET}")
    endif ()
    if (ARGS_PROJECT_ROOT STREQUAL "")
        message(FATAL_ERROR "erbsland_core_add_git_version requires PROJECT_ROOT.")
    endif ()
    if (ARGS_TEMPLATE STREQUAL "")
        message(FATAL_ERROR "erbsland_core_add_git_version requires TEMPLATE.")
    endif ()

    erbsland_core_resolve_path(project_root "${ARGS_PROJECT_ROOT}" "${CMAKE_CURRENT_SOURCE_DIR}")
    erbsland_core_resolve_path(template_path "${ARGS_TEMPLATE}" "${CMAKE_CURRENT_SOURCE_DIR}")
    if (NOT IS_DIRECTORY "${project_root}")
        message(FATAL_ERROR "erbsland_core_add_git_version PROJECT_ROOT does not exist: ${project_root}")
    endif ()
    if (NOT EXISTS "${template_path}")
        message(FATAL_ERROR "erbsland_core_add_git_version TEMPLATE does not exist: ${ARGS_TEMPLATE}")
    endif ()

    set(variable_prefix "${ARGS_VARIABLE_PREFIX}")
    if (variable_prefix STREQUAL "")
        set(variable_prefix "ERBSLAND_GIT_VERSION")
    endif ()
    if (NOT variable_prefix MATCHES "^[A-Za-z_][A-Za-z0-9_]*$")
        message(FATAL_ERROR "erbsland_core_add_git_version VARIABLE_PREFIX is not a valid CMake variable prefix.")
    endif ()

    set(output_path "${ARGS_OUTPUT}")
    if (output_path STREQUAL "")
        get_filename_component(template_name "${template_path}" NAME)
        string(REGEX REPLACE "\\.in(\\.[^.]+)$" "\\1" output_name "${template_name}")
        set(output_path "${CMAKE_CURRENT_BINARY_DIR}/${ARGS_TARGET}-git-version/${output_name}")
    else ()
        erbsland_core_resolve_path(output_path "${output_path}" "${CMAKE_CURRENT_BINARY_DIR}")
    endif ()

    erbsland_core_collect_git_dependencies(git_dependencies "${project_root}")
    add_custom_command(
            OUTPUT "${output_path}"
            COMMAND "${CMAKE_COMMAND}"
                    "-DERBSLAND_CORE_SOURCE_DIR=${project_root}"
                    "-DERBSLAND_CORE_TEMPLATE=${template_path}"
                    "-DERBSLAND_CORE_OUTPUT=${output_path}"
                    "-DERBSLAND_GIT_VERSION_VARIABLE_PREFIX=${variable_prefix}"
                    -P "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
            DEPENDS
                    "${template_path}"
                    "${CMAKE_CURRENT_FUNCTION_LIST_FILE}"
                    ${git_dependencies}
            VERBATIM
    )
    set(version_target "${ARGS_TARGET}-git-version")
    add_custom_target("${version_target}" DEPENDS "${output_path}")
    add_dependencies("${ARGS_TARGET}" "${version_target}")
    target_sources(${ARGS_TARGET} PRIVATE "${output_path}")
endfunction()

if (CMAKE_SCRIPT_MODE_FILE)
    erbsland_core_generate_version_source()
endif ()
