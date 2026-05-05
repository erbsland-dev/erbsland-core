# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

if(ERBSLAND_CORE_ENABLE_TESTS)
    add_subdirectory(test)

    enable_testing()
    add_test(
            NAME erbsland-core-unittest
            COMMAND $<TARGET_FILE:erbsland-core-unittest> --no-color
    )
endif()
