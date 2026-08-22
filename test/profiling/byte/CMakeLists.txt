# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET byte-types-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE BYTE_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(src)

add_test(
        NAME byte-types-profile-dry-run
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/smoke.elcl --dry-run
)
add_test(
        NAME byte-types-profile-smoke-one-thread
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:byte-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/byte/config/smoke.elcl
                -DTHREADS=1
                -P ${PROJECT_SOURCE_DIR}/test/profiling/byte/cmake/VerifySmoke.cmake
)
add_test(
        NAME byte-types-profile-smoke-four-threads
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:byte-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/byte/config/smoke.elcl
                -DTHREADS=4
                -P ${PROJECT_SOURCE_DIR}/test/profiling/byte/cmake/VerifySmoke.cmake
)
add_test(
        NAME byte-types-profile-coverage-map
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:byte-types-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/byte/cmake/VerifyCoverage.cmake
)
add_test(
        NAME byte-types-profile-all-paths
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/coverage.elcl
)
add_test(
        NAME byte-types-profile-custom-configuration
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/custom.elcl --dry-run
)
add_test(
        NAME byte-types-profile-invalid-configuration
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/invalid.elcl --dry-run
)
add_test(
        NAME byte-types-profile-unknown-key
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/invalid-unknown.elcl --dry-run
)
add_test(
        NAME byte-types-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:byte-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/byte/config/custom.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/byte/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME byte-types-profile-deadline
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/deadline.elcl
)
add_test(
        NAME byte-types-profile-worker-failure
        COMMAND $<TARGET_FILE:byte-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/byte/config/smoke.elcl --threads 4 --type byte-array
)
set_tests_properties(
        byte-types-profile-dry-run
        byte-types-profile-smoke-one-thread
        byte-types-profile-smoke-four-threads
        byte-types-profile-coverage-map
        byte-types-profile-all-paths
        byte-types-profile-custom-configuration
        byte-types-profile-invalid-configuration
        byte-types-profile-unknown-key
        byte-types-profile-determinism
        byte-types-profile-deadline
        byte-types-profile-worker-failure
        PROPERTIES TIMEOUT 60
)
set_tests_properties(
        byte-types-profile-invalid-configuration
        byte-types-profile-unknown-key
        byte-types-profile-deadline
        byte-types-profile-worker-failure
        PROPERTIES WILL_FAIL TRUE
)
set_tests_properties(byte-types-profile-worker-failure PROPERTIES
        ENVIRONMENT "ERBSLAND_BYTE_PROFILE_TEST_FAIL_WORKER=1"
)
