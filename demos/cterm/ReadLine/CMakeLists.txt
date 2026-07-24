# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(read-line
        src/ReadLineApp.cpp
        src/ReadLineApp.hpp
        src/ReadLineApp_configuration.cpp
        src/ReadLineApp_dump.cpp
        src/ReadLineApp_options.cpp
        src/ReadLineApp_style.cpp
        src/main.cpp
)
