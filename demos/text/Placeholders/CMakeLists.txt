# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(text_placeholders)
target_sources(text_placeholders PRIVATE
        BuiltInFilters.cpp
        BuiltInSources.cpp
        CustomFilter.cpp
        CustomSource.cpp
        PlaceholderDemos.cpp
        PlaceholderDemos.hpp
        main.cpp
)
