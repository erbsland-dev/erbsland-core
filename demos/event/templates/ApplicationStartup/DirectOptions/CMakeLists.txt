# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo_template(startup_direct_options)
target_sources(startup_direct_options PRIVATE
        Work.hpp
        main.cpp
)
