# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(access_checks)
target_sources(access_checks PRIVATE
        AccessChecksDemos.hpp
        ConfigureFileAccess.cpp
        CustomAccessCheck.cpp
        main.cpp
)
