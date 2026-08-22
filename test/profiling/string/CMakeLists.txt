# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET string-types-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE STRING_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${PROJECT_SOURCE_DIR}/src"
)

add_subdirectory(src)

add_test(
        NAME string-types-profile-dry-run
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/smoke.elcl --dry-run
)
add_test(
        NAME string-types-profile-smoke-one-thread
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:string-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/string/config/smoke.elcl
                -DTHREADS=1
                -P ${PROJECT_SOURCE_DIR}/test/profiling/string/cmake/VerifySmoke.cmake
)
add_test(
        NAME string-types-profile-smoke-four-threads
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:string-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/string/config/smoke.elcl
                -DTHREADS=4
                -P ${PROJECT_SOURCE_DIR}/test/profiling/string/cmake/VerifySmoke.cmake
)
add_test(
        NAME string-types-profile-coverage-map
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:string-types-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/string/cmake/VerifyCoverage.cmake
)
add_test(
        NAME string-types-profile-all-paths
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/coverage.elcl
)
add_test(
        NAME string-types-profile-custom-configuration
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/custom.elcl --dry-run
)
add_test(
        NAME string-types-profile-invalid-configuration
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/invalid.elcl --dry-run
)
add_test(
        NAME string-types-profile-unknown-key
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/invalid-unknown.elcl --dry-run
)
add_test(
        NAME string-types-profile-incompatible-configuration
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/invalid-incompatible.elcl --dry-run
)
add_test(
        NAME string-types-profile-duplicate-scenario
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/invalid-duplicate.elcl --dry-run
)
add_test(
        NAME string-types-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:string-types-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/string/config/custom.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/string/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME string-types-profile-sampling-mode
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/profile-smoke.elcl
)
add_test(
        NAME string-types-profile-write-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:string-types-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/string-types-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/string/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/string/cmake/VerifyTemplate.cmake
)
add_test(
        NAME string-types-profile-deadline
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/deadline.elcl
)
add_test(
        NAME string-types-profile-worker-failure
        COMMAND $<TARGET_FILE:string-types-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/string/config/smoke.elcl --threads 4 --width u8
)
set_tests_properties(
        string-types-profile-dry-run
        string-types-profile-smoke-one-thread
        string-types-profile-smoke-four-threads
        string-types-profile-coverage-map
        string-types-profile-all-paths
        string-types-profile-custom-configuration
        string-types-profile-invalid-configuration
        string-types-profile-unknown-key
        string-types-profile-incompatible-configuration
        string-types-profile-duplicate-scenario
        string-types-profile-determinism
        string-types-profile-sampling-mode
        string-types-profile-write-template
        string-types-profile-deadline
        string-types-profile-worker-failure
        PROPERTIES TIMEOUT 60
)
set_tests_properties(
        string-types-profile-invalid-configuration
        string-types-profile-unknown-key
        string-types-profile-incompatible-configuration
        string-types-profile-duplicate-scenario
        string-types-profile-deadline
        string-types-profile-worker-failure
        PROPERTIES WILL_FAIL TRUE
)
set_tests_properties(string-types-profile-worker-failure PROPERTIES
        ENVIRONMENT "ERBSLAND_STRING_PROFILE_TEST_FAIL_WORKER=1"
)
