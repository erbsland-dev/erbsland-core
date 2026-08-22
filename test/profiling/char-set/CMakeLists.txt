# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET char-set-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE CHAR_SET_PROFILE_DEFAULT_CONFIGURATION
)

add_subdirectory(src)

add_test(
        NAME char-set-profile-smoke
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:char-set-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/char-set/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifySmoke.cmake
)
add_test(
        NAME char-set-profile-dry-run
        COMMAND $<TARGET_FILE:char-set-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/char-set/config/smoke.elcl --dry-run
)
add_test(
        NAME char-set-profile-coverage-map
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:char-set-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/char-set/cmake/VerifyCoverage.cmake
)
add_test(
        NAME char-set-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:char-set-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/char-set/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME char-set-profile-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:char-set-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/char-set-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/char-set/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyTemplate.cmake
)
