# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(base_n)
target_sources(base_n PRIVATE
        BaseNDemos.cpp
        BaseNDemos.hpp
        main.cpp
)
