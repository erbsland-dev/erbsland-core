# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET stream-file-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE STREAM_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(src)

add_test(
        NAME stream-file-profile-dry-run
        COMMAND $<TARGET_FILE:stream-file-profile> --dry-run
)
add_test(
        NAME stream-file-profile-smoke-one-thread
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:stream-file-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/stream/config/smoke.elcl
                -DTHREADS=1
                -P ${PROJECT_SOURCE_DIR}/test/profiling/stream/cmake/VerifySmoke.cmake
)
add_test(
        NAME stream-file-profile-smoke-four-threads
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:stream-file-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/stream/config/smoke.elcl
                -DTHREADS=4
                -P ${PROJECT_SOURCE_DIR}/test/profiling/stream/cmake/VerifySmoke.cmake
)
add_test(
        NAME stream-file-profile-custom-configuration
        COMMAND $<TARGET_FILE:stream-file-profile>
                --config ${PROJECT_SOURCE_DIR}/test/profiling/stream/config/custom.elcl
                --dry-run
)
add_test(
        NAME stream-file-profile-invalid-configuration
        COMMAND $<TARGET_FILE:stream-file-profile>
                --config ${PROJECT_SOURCE_DIR}/test/profiling/stream/config/invalid.elcl
                --dry-run
)
add_test(
        NAME stream-file-profile-unknown-key
        COMMAND $<TARGET_FILE:stream-file-profile>
                --config ${PROJECT_SOURCE_DIR}/test/profiling/stream/config/invalid-unknown.elcl
                --dry-run
)
add_test(
        NAME stream-file-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:stream-file-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/stream/config/custom.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/stream/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME stream-file-profile-statistics
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:stream-file-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/stream/config/benchmark.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/stream/cmake/VerifyBenchmark.cmake
)
add_test(
        NAME stream-file-profile-deadline
        COMMAND $<TARGET_FILE:stream-file-profile>
                --config ${PROJECT_SOURCE_DIR}/test/profiling/stream/config/deadline.elcl
                --scenario binary:read-byte:binary:fixed:hot:buffer-4096
)
add_test(
        NAME stream-file-profile-worker-failure
        COMMAND $<TARGET_FILE:stream-file-profile>
                --config ${PROJECT_SOURCE_DIR}/test/profiling/stream/config/smoke.elcl
                --scenario binary:read-byte:binary:fixed:hot:buffer-4096
                --threads 4
)
set_tests_properties(
        stream-file-profile-dry-run
        stream-file-profile-smoke-one-thread
        stream-file-profile-smoke-four-threads
        stream-file-profile-custom-configuration
        stream-file-profile-invalid-configuration
        stream-file-profile-unknown-key
        stream-file-profile-determinism
        stream-file-profile-statistics
        stream-file-profile-deadline
        stream-file-profile-worker-failure
        PROPERTIES TIMEOUT 60
)
set_tests_properties(
        stream-file-profile-invalid-configuration
        stream-file-profile-unknown-key
        stream-file-profile-deadline
        stream-file-profile-worker-failure
        PROPERTIES WILL_FAIL TRUE
)
set_tests_properties(stream-file-profile-worker-failure PROPERTIES
        ENVIRONMENT "ERBSLAND_STREAM_PROFILE_TEST_FAIL_WORKER=1"
)
