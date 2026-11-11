# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(erbsland-core-file-search-unittest)
add_subdirectory(src)
target_link_libraries(erbsland-core-file-search-unittest PRIVATE erbsland-core-file-search-support)
erbsland_set_required_compiler_options(erbsland-core-file-search-unittest)
erbsland_core_configure_unittest(erbsland-core-file-search-unittest ENABLE_WARNINGS)
set_target_properties(erbsland-core-file-search-unittest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/demos/event/FileSearch/test"
)
add_test(NAME erbsland-core-file-search-unittest COMMAND erbsland-core-file-search-unittest --no-color)

if(UNIX)
    add_test(NAME file-search-terminal
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/terminal_test.py" $<TARGET_FILE:file_search>)
    set_tests_properties(file-search-terminal PROPERTIES TIMEOUT 30)
endif()
