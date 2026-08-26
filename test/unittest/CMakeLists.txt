# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-unittest)
add_dependencies(erbsland-core-unittest erbsland-core-resource-integration)
erbsland_set_required_compiler_options(erbsland-core-unittest)
target_link_libraries(erbsland-core-unittest PRIVATE erbsland::core)
if (MSVC)
    # Deep dependency-limit tests exercise intentionally recursive debug code whose /RTC stack frames exceed 1 MiB.
    target_link_options(erbsland-core-unittest PRIVATE /STACK:4194304)
endif()
target_include_directories(erbsland-core-unittest PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        src
)

target_compile_definitions(erbsland-core PRIVATE
        ERBSLAND_UNITTEST_BUILD=1
        ERBSLAND_CORE_CONF_INTERNAL_VIEWS=1
)
target_compile_definitions(erbsland-core-unittest PRIVATE
        ERBSLAND_UNITTEST_BUILD=1
        ERBSLAND_CORE_CONF_INTERNAL_VIEWS=1
)

if (CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    string(TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" _erbsland_unittest_aes_processor)
else()
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _erbsland_unittest_aes_processor)
endif()
if (_erbsland_unittest_aes_processor MATCHES "^(arm64|aarch64)$")
    target_compile_definitions(erbsland-core-unittest PRIVATE
            ERBSLAND_AES_ARM_BACKEND=1
            ERBSLAND_CHACHA20_ARM_BACKEND=1
    )
elseif (_erbsland_unittest_aes_processor MATCHES "^(amd64|x64|x86_64)$")
    target_compile_definitions(erbsland-core-unittest PRIVATE
            ERBSLAND_AES_X86_BACKEND=1
            ERBSLAND_CHACHA20_X86_BACKEND=1
    )
endif()
add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-unittest
        COPY_TEST_DATA data
        ENABLE_DATA_DEPS
)

add_test(
        NAME erbsland-core-unittest
        COMMAND $<TARGET_FILE:erbsland-core-unittest> --no-color
)
set_tests_properties(erbsland-core-unittest PROPERTIES
        ENVIRONMENT "ERBSLAND_CORE_CONF_TEST_SUITE=${PROJECT_SOURCE_DIR}/test/erbsland-lang-config-tests"
)
