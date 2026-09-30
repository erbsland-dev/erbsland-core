# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(json_values)
target_sources(json_values PRIVATE
        JsonValuesDemos.hpp
        main.cpp
        ParseDocument.cpp
)
