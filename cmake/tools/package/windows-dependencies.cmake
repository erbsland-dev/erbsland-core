# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0
cmake_minimum_required(VERSION 3.28)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()
file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES "${PACKAGE_EXECUTABLE}"
        DIRECTORIES ${PACKAGE_LIBRARY_DIRECTORIES}
        RESOLVED_DEPENDENCIES_VAR resolved
        UNRESOLVED_DEPENDENCIES_VAR unresolved
        CONFLICTING_DEPENDENCIES_PREFIX conflict
        PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
        POST_EXCLUDE_REGEXES
                "[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/[Ss][Yy][Ss][Tt][Ee][Mm]32/"
                "[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/[Ss][Yy][Ss][Ww][Oo][Ww]64/"
                "[Ww][Ii][Nn][Dd][Oo][Ww][Ss]/[Ww][Ii][Nn][Ss][Xx][Ss]/")
if(unresolved)
    message(FATAL_ERROR "Unresolved runtime dependencies: ${unresolved}")
endif()
if(conflict_FILENAMES)
    message(FATAL_ERROR "Conflicting runtime dependencies: ${conflict_FILENAMES}")
endif()
set(output "")
foreach(dependency IN LISTS resolved)
    file(TO_CMAKE_PATH "${dependency}" normalized)
    string(TOLOWER "${normalized}" normalized)
    if(NOT normalized MATCHES "^[a-z]:/windows/(system32|winsxs)(/|$)")
        string(APPEND output "${dependency}\n")
    endif()
endforeach()
file(WRITE "${PACKAGE_DEPENDENCY_OUTPUT}" "${output}")
