# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-security_hashes PRIVATE
        Configuration.hpp
        Input.hpp
        Inventory.cpp
        Inventory.hpp
        Manifest.cpp
        Manifest.hpp
        SecurityHashesTool.cpp
        SecurityHashesTool.hpp
        Settings.cpp
        Settings.hpp
)
