# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(placeholders)
target_sources(placeholders PRIVATE
        BuiltInFilters.cpp
        BuiltInProviders.cpp
        BuiltInSources.cpp
        CustomFilter.cpp
        CustomSource.cpp
        ErrorHandling.cpp
        main.cpp
        PlaceholdersDemos.hpp
)
