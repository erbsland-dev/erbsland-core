# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET cryptology-hash-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE CRYPTOLOGY_PROFILE_DEFAULT_CONFIGURATION
)

add_subdirectory(src)

add_test(
        NAME cryptology-hash-profile-smoke
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:cryptology-hash-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifySmoke.cmake
)
add_test(
        NAME cryptology-hash-profile-dry-run
        COMMAND $<TARGET_FILE:cryptology-hash-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/smoke.elcl --dry-run
)
add_test(
        NAME cryptology-hash-profile-invalid-configuration
        COMMAND $<TARGET_FILE:cryptology-hash-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/invalid.elcl --dry-run
)
set_tests_properties(cryptology-hash-profile-invalid-configuration PROPERTIES WILL_FAIL TRUE)
add_test(
        NAME cryptology-hash-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:cryptology-hash-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME cryptology-hash-profile-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:cryptology-hash-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/cryptology-hash-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyTemplate.cmake
)
add_test(
        NAME cryptology-hash-profile-profile-smoke
        COMMAND $<TARGET_FILE:cryptology-hash-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/profile-smoke.elcl
)
add_test(
        NAME cryptology-hash-profile-deadline
        COMMAND $<TARGET_FILE:cryptology-hash-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/deadline.elcl
)
set_tests_properties(cryptology-hash-profile-deadline PROPERTIES WILL_FAIL TRUE)
add_test(
        NAME cryptology-hash-profile-worker-failure
        COMMAND $<TARGET_FILE:cryptology-hash-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/cryptology/config/worker-failure.elcl
)
set_tests_properties(cryptology-hash-profile-worker-failure PROPERTIES WILL_FAIL TRUE)
