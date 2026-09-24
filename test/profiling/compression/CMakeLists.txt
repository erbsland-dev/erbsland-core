# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

set(COMPRESSION_PROFILE_CORPUS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data")
configure_file(
        src/CorpusDirectory.hpp.in
        generated/CorpusDirectory.hpp
        @ONLY
)

erbsland_add_profiling_executable(
    TARGET compression-profile
    DEFAULT_CONFIGURATION config/default.elcl
    DEFAULT_HEADER_TEMPLATE src/DefaultConfiguration.hpp.in
    DEFAULT_VARIABLE COMPRESSION_PROFILE_DEFAULT_CONFIGURATION
    INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

add_subdirectory(src)

add_test(
        NAME compression-profile-smoke
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:compression-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/compression/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifySmoke.cmake
)
add_test(
        NAME compression-profile-dry-run
        COMMAND $<TARGET_FILE:compression-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/compression/config/smoke.elcl --dry-run
)
add_test(
        NAME compression-profile-determinism
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:compression-profile>
                -DCONFIGURATION=${PROJECT_SOURCE_DIR}/test/profiling/compression/config/smoke.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyDeterminism.cmake
)
add_test(
        NAME compression-profile-template
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:compression-profile>
                -DOUTPUT=${CMAKE_BINARY_DIR}/compression-profile-template.elcl
                -DEXPECTED=${PROJECT_SOURCE_DIR}/test/profiling/compression/config/default.elcl
                -P ${PROJECT_SOURCE_DIR}/test/profiling/framework/cmake/VerifyTemplate.cmake
)
add_test(
        NAME compression-profile-coverage
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:compression-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/compression/cmake/VerifyCoverage.cmake
)
add_test(
        NAME compression-profile-ratios
        COMMAND ${CMAKE_COMMAND}
                -DEXECUTABLE=$<TARGET_FILE:compression-profile>
                -P ${PROJECT_SOURCE_DIR}/test/profiling/compression/cmake/VerifyRatios.cmake
)
add_test(
        NAME compression-profile-corpus-digests
        COMMAND $<TARGET_FILE:compression-profile>
                --suite snapshot --algorithm zstandard
                --corpus matches --corpus sparse-binary --corpus entropy --corpus mixed
                --samples 1 --warmup-samples 0 --minimum-sample-time 100us
)
add_test(
        NAME compression-profile-invalid-incompatible
        COMMAND $<TARGET_FILE:compression-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/compression/config/invalid-incompatible.elcl --dry-run
)
add_test(
        NAME compression-profile-invalid-unknown
        COMMAND $<TARGET_FILE:compression-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/compression/config/invalid-unknown.elcl --dry-run
)
add_test(
        NAME compression-profile-invalid-incomplete
        COMMAND $<TARGET_FILE:compression-profile> --config
                ${PROJECT_SOURCE_DIR}/test/profiling/compression/config/invalid-incomplete.elcl --dry-run
)
set_tests_properties(
        compression-profile-invalid-incompatible
        compression-profile-invalid-unknown
        compression-profile-invalid-incomplete
        PROPERTIES WILL_FAIL TRUE
)
set_tests_properties(
        compression-profile-smoke
        compression-profile-dry-run
        compression-profile-determinism
        compression-profile-template
        compression-profile-coverage
        compression-profile-corpus-digests
        compression-profile-ratios
        compression-profile-invalid-incompatible
        compression-profile-invalid-unknown
        compression-profile-invalid-incomplete
        PROPERTIES TIMEOUT 120
)
