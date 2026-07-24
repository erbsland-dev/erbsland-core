# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

configure_file(
        src/BuildIdentity.hpp.in
        generated/BuildIdentity.hpp
        @ONLY
)
configure_file(
        src/DefaultConfiguration.hpp.in
        generated/DefaultConfiguration.hpp
        @ONLY
)

set(REGEX_PROFILE_CORPUS_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/data")
file(MAKE_DIRECTORY "${REGEX_PROFILE_CORPUS_DIRECTORY}")
configure_file(
        "${PROJECT_SOURCE_DIR}/test/unittest/data/re/shakespeare.txt"
        "${REGEX_PROFILE_CORPUS_DIRECTORY}/shakespeare.txt"
        COPYONLY
)
configure_file(
        "${PROJECT_SOURCE_DIR}/test/unittest/data/re/shakespeare.html"
        "${REGEX_PROFILE_CORPUS_DIRECTORY}/shakespeare.html"
        COPYONLY
)
configure_file(
        src/CorpusDirectory.hpp.in
        generated/CorpusDirectory.hpp
        @ONLY
)

erbsland_add_profiling_executable(
    TARGET regex-api-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE REGEX_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(src)
