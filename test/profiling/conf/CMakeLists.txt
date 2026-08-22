# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_add_profiling_executable(
    TARGET conf-parser-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE CONF_PROFILE_DEFAULT_CONFIGURATION
)

add_subdirectory(src)

add_test(
        NAME conf-parser-profile-smoke
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:conf-parser-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/conf/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifySmoke.cmake
)
add_test(
        NAME conf-parser-profile-dry-run
        COMMAND $<TARGET_FILE:conf-parser-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/conf/config/smoke.elcl --dry-run
)
add_test(
        NAME conf-parser-profile-invalid-configuration
        COMMAND $<TARGET_FILE:conf-parser-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/conf/config/invalid.elcl --dry-run
)
set_tests_properties(conf-parser-profile-invalid-configuration PROPERTIES WILL_FAIL TRUE)
add_test(
        NAME conf-parser-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:conf-parser-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/conf/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME conf-parser-profile-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:conf-parser-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/conf-parser-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/conf/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyTemplate.cmake
)
add_test(
        NAME conf-parser-profile-profile-smoke
        COMMAND $<TARGET_FILE:conf-parser-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/conf/config/profile-smoke.elcl
)
add_test(
        NAME conf-parser-profile-deadline
        COMMAND $<TARGET_FILE:conf-parser-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/conf/config/deadline.elcl
)
set_tests_properties(conf-parser-profile-deadline PROPERTIES WILL_FAIL TRUE)
add_test(
        NAME conf-parser-profile-worker-failure
        COMMAND $<TARGET_FILE:conf-parser-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/conf/config/worker-failure.elcl
)
set_tests_properties(conf-parser-profile-worker-failure PROPERTIES WILL_FAIL TRUE)
