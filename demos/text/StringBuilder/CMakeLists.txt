# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(string_builder)
target_sources(string_builder PRIVATE
        FieldGuideCards.cpp
        main.cpp
        StringBuilderDemos.hpp
)
