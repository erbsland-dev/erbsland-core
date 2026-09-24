# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(source_resolving)
target_sources(source_resolving PRIVATE
        ConfigureFileResolver.cpp
        CustomResolver.cpp
        DefaultFileResolver.cpp
        main.cpp
        SourceResolvingDemos.hpp
)
