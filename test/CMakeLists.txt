# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

if (ERBSLAND_CORE_ENABLE_TESTS)
    if (NOT TARGET erbsland-unittest)
        add_subdirectory(erbsland-unittest EXCLUDE_FROM_ALL)
    endif ()
    add_subdirectory(adapter)
    add_subdirectory(unittest)

    add_subdirectory(cmake)
    add_subdirectory(heap)
    add_subdirectory(profiling)
endif ()

if (ERBSLAND_CORE_ENABLE_INTEROP_TESTS)
    add_subdirectory(interop)
endif ()

if(ERBSLAND_CORE_ENABLE_FUZZING)
    add_subdirectory(fuzzing)
endif()
