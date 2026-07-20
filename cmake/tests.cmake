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
            NAME conf-parser-profile-smoke
            COMMAND $<TARGET_FILE:conf-parser-profile> --iterations 1
    )
    set_tests_properties(erbsland-core-unittest PROPERTIES
            ENVIRONMENT "ERBSLAND_CORE_CONF_TEST_SUITE=${PROJECT_SOURCE_DIR}/test/erbsland-lang-config-tests"
    )
endif()
