# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

function(erbsland_core_configure_unittest target)
    set(_options)
    if(ERBSLAND_CORE_ENABLE_PRECOMPILED_HEADERS)
        list(APPEND _options PRECOMPILE_HEADERS)
    endif()
    erbsland_enable_precompiled_headers(${target})
    erbsland_unittest(TARGET ${target} ${_options} ${ARGN})
endfunction()

if(ERBSLAND_CORE_ENABLE_TESTS)
    add_subdirectory(test)

    enable_testing()
    add_test(
            NAME erbsland-core-unittest
            COMMAND $<TARGET_FILE:erbsland-core-unittest> --no-color
    )
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
    add_test(
            NAME erbsland-profiling-unittest
            COMMAND $<TARGET_FILE:erbsland-profiling-unittest> --no-color
    )
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
    add_test(
            NAME stream-file-profile-dry-run
            COMMAND $<TARGET_FILE:stream-file-profile> --dry-run
    )
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
            regex-api-profile-sampling-mode
            regex-api-profile-write-template
            regex-api-profile-deadline
            regex-api-profile-worker-failure
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
            byte-types-profile-invalid-configuration
            byte-types-profile-unknown-key
            byte-types-profile-deadline
            byte-types-profile-worker-failure
            string-types-profile-invalid-configuration
            string-types-profile-unknown-key
            string-types-profile-incompatible-configuration
            string-types-profile-duplicate-scenario
            string-types-profile-deadline
            string-types-profile-worker-failure
            regex-api-profile-invalid-configuration
            regex-api-profile-unknown-key
            regex-api-profile-incompatible-configuration
            regex-api-profile-duplicate-scenario
            regex-api-profile-deadline
            regex-api-profile-worker-failure
            stream-file-profile-invalid-configuration
            stream-file-profile-unknown-key
            stream-file-profile-deadline
            stream-file-profile-worker-failure
            PROPERTIES WILL_FAIL TRUE
    )
    set_tests_properties(byte-types-profile-worker-failure PROPERTIES
            ENVIRONMENT "ERBSLAND_BYTE_PROFILE_TEST_FAIL_WORKER=1"
    )
    set_tests_properties(string-types-profile-worker-failure PROPERTIES
            ENVIRONMENT "ERBSLAND_STRING_PROFILE_TEST_FAIL_WORKER=1"
    )
    set_tests_properties(stream-file-profile-worker-failure PROPERTIES
            ENVIRONMENT "ERBSLAND_STREAM_PROFILE_TEST_FAIL_WORKER=1"
    )
    set_tests_properties(regex-api-profile-worker-failure PROPERTIES
            ENVIRONMENT "ERBSLAND_REGEX_PROFILE_TEST_FAIL_WORKER=1"
    )
    set_tests_properties(erbsland-core-unittest PROPERTIES
            ENVIRONMENT "ERBSLAND_CORE_CONF_TEST_SUITE=${PROJECT_SOURCE_DIR}/test/erbsland-lang-config-tests"
    )
endif()
