# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_executable(path-profile-unittest)
add_subdirectory(src)
target_link_libraries(path-profile-unittest PRIVATE path-profile-workloads)
target_compile_features(path-profile-unittest PRIVATE cxx_std_20)
erbsland_set_required_compiler_options(path-profile-unittest)
erbsland_enable_debug_warnings(path-profile-unittest)
erbsland_unittest(TARGET path-profile-unittest ENABLE_WARNINGS)

add_test(NAME path-profile-unittest COMMAND $<TARGET_FILE:path-profile-unittest> --no-color)
set_tests_properties(path-profile-unittest PROPERTIES LABELS ci)

foreach(check registration read-one read-two errors traversal)
    add_test(
            NAME path-profile-${check}
            COMMAND ${CMAKE_COMMAND}
                    -DEXECUTABLE=$<TARGET_FILE:path-walker-profile>
                    -DSOURCE_DIRECTORY=${PROJECT_SOURCE_DIR}/test/profiling/path
                    -DWORK_DIRECTORY=${CMAKE_CURRENT_BINARY_DIR}/${check}
                    -DCHECK=${check}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/VerifySourceTree.cmake
    )
    set_tests_properties(path-profile-${check} PROPERTIES TIMEOUT 120)
endforeach()
