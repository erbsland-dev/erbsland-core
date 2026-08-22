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

add_test(
        NAME regex-api-profile-dry-run
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/smoke.elcl --dry-run
)
add_test(
        NAME regex-api-profile-smoke-one-thread
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/regex/config/smoke.elcl
                -DTHREADS=1
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifySmoke.cmake
)
add_test(
        NAME regex-api-profile-smoke-four-threads
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/regex/config/smoke.elcl
                -DTHREADS=4
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifySmoke.cmake
)
add_test(
        NAME regex-api-profile-coverage-map
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyCoverage.cmake
)
add_test(
        NAME regex-api-profile-all-paths
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/coverage.elcl
)
add_test(
        NAME regex-api-profile-custom-configuration
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/custom.elcl --dry-run
)
add_test(
        NAME regex-api-profile-invalid-configuration
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/invalid.elcl --dry-run
)
add_test(
        NAME regex-api-profile-unknown-key
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/invalid-unknown.elcl --dry-run
)
add_test(
        NAME regex-api-profile-incompatible-configuration
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/invalid-incompatible.elcl --dry-run
)
add_test(
        NAME regex-api-profile-duplicate-scenario
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/invalid-duplicate.elcl --dry-run
)
add_test(
        NAME regex-api-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/regex/config/custom.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME regex-api-profile-backend-parity
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/regex/config/coverage.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyParity.cmake
)
add_test(
        NAME regex-api-profile-command-line-overrides
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyOverrides.cmake
)
add_test(
        NAME regex-api-profile-std-comparison
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyComparison.cmake
)
add_test(
        NAME regex-api-profile-sampling-mode
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/profile-smoke.elcl
)
add_test(
        NAME regex-api-profile-write-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:regex-api-profile>
                -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/regex-api-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/regex/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/regex/cmake/VerifyTemplate.cmake
)
add_test(
        NAME regex-api-profile-deadline
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/deadline.elcl
)
add_test(
        NAME regex-api-profile-worker-failure
        COMMAND $<TARGET_FILE:regex-api-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/regex/config/smoke.elcl --threads 4 --use-case compile
)
set_tests_properties(
        regex-api-profile-dry-run
        regex-api-profile-smoke-one-thread
        regex-api-profile-smoke-four-threads
        regex-api-profile-coverage-map
        regex-api-profile-all-paths
        regex-api-profile-custom-configuration
        regex-api-profile-invalid-configuration
        regex-api-profile-unknown-key
        regex-api-profile-incompatible-configuration
        regex-api-profile-duplicate-scenario
        regex-api-profile-determinism
        regex-api-profile-backend-parity
        regex-api-profile-command-line-overrides
        regex-api-profile-std-comparison
        regex-api-profile-sampling-mode
        regex-api-profile-write-template
        regex-api-profile-deadline
        regex-api-profile-worker-failure
        PROPERTIES TIMEOUT 60
)
set_tests_properties(
        regex-api-profile-invalid-configuration
        regex-api-profile-unknown-key
        regex-api-profile-incompatible-configuration
        regex-api-profile-duplicate-scenario
        regex-api-profile-deadline
        regex-api-profile-worker-failure
        PROPERTIES WILL_FAIL TRUE
)
set_tests_properties(regex-api-profile-worker-failure PROPERTIES
        ENVIRONMENT "ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER=1"
)
