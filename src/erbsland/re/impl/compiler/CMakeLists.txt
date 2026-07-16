# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CodeGenerator.cpp
        CodeGenerator.hpp
        CodeGenerator_jumps.cpp
        CodeGenerator_nodes.cpp
        CodeGenerator_quantifiers.cpp
        Compiler.cpp
        Compiler.hpp
        DataCollector.hpp
)
