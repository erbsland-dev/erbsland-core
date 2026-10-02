# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

# Deterministic on-demand checks link the same harness and embedded resources as the application.
add_executable(erbsland-constant-time-unittest EXCLUDE_FROM_ALL)
erbsland_core_setup_application(TARGET erbsland-constant-time-unittest)
target_link_libraries(erbsland-constant-time-unittest PRIVATE erbsland-constant-time)
set_target_properties(erbsland-constant-time-unittest PROPERTIES
        UNITY_BUILD OFF
        INTERPROCEDURAL_OPTIMIZATION OFF
)

add_subdirectory(src)

# Discover metadata after the source list is complete, without registering statistical or harness CTest runs.
erbsland_unittest(TARGET erbsland-constant-time-unittest ENABLE_WARNINGS)
