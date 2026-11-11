# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(file_server)
target_sources(file_server PRIVATE
        ServerPart.cpp
        ServerPart.hpp
        ServerStatus.hpp
        main.cpp
)
