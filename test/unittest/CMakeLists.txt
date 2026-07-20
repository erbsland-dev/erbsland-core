# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-unittest)
erbsland_set_required_compiler_options(erbsland-core-unittest)
target_link_libraries(erbsland-core-unittest PRIVATE erbsland::core)
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
add_subdirectory(src)

erbsland_core_configure_unittest(erbsland-core-unittest
        COPY_TEST_DATA data
        ENABLE_DATA_DEPS
)
