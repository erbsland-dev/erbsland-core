# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

set(RENDER_PROFILE_CORPUS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data")
configure_file(
        src/CorpusDirectory.hpp.in
        generated/CorpusDirectory.hpp
        @ONLY
)

erbsland_add_profiling_executable(
    TARGET render-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE RENDER_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(src)

add_test(
        NAME render-profile-smoke
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:render-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/render/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifySmoke.cmake
)
add_test(
        NAME render-profile-dry-run
        COMMAND $<TARGET_FILE:render-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/render/config/smoke.elcl --dry-run
)
add_test(
        NAME render-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:render-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/render/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME render-profile-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:render-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/render-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/render/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyTemplate.cmake
)
add_test(
        NAME render-profile-coverage
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:render-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/render/cmake/VerifyCoverage.cmake
)

set_tests_properties(
        render-profile-smoke
        render-profile-dry-run
        render-profile-determinism
        render-profile-template
        render-profile-coverage
        PROPERTIES TIMEOUT 60
)
