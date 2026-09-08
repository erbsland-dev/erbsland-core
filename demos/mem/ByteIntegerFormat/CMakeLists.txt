# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(byte_integer_format)
target_sources(byte_integer_format PRIVATE
        ByteIntegerFormatDemos.hpp
        FixedFormats.cpp
        main.cpp
        SignedVariableLength.cpp
        UnsignedBase128.cpp
        UnsignedVariableLength.cpp
)
