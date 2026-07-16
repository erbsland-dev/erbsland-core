# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        ArgumentTest.cpp
        AssemblerAnchorTest.cpp
        AssemblerBase.hpp
        AssemblerBasicTest.cpp
        AssemblerCaptureTest.cpp
        AssemblerCategoryTest.cpp
        AssemblerCharTest.cpp
        AssemblerCounterTest.cpp
        AssemblerFlowTest.cpp
        AssemblerMatchTest.cpp
        AssemblerTokenizerTest.cpp
        AssemblerTokenTest.cpp
        DataSectionTest.cpp
        DisassemblerTest.cpp
)
