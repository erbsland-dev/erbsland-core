# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_text_options)
target_sources(byte_text_options PRIVATE
        ByteTextOptionsDemos.hpp
        main.cpp
        SelectCountFormat.cpp
        SelectEncoding.cpp
        SelectFormat.cpp
        UseEndMark.cpp
        UsePadding.cpp
)
