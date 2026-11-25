# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-core PRIVATE
        BoxAttributes.hpp
        BoxCharacter.hpp
        BoxFrameData.cpp
        BoxFrameData.hpp
        BoxFrameSource.cpp
        BoxFrameSource.hpp
        CommonBoxFrameStyleTool.cpp
        CommonBoxFrameStyleTool.hpp
        FrameBorderJointCharsTool.cpp
        FrameBorderJointCharsTool.hpp
        FrameGeneratorTool.cpp
        FrameGeneratorTool.hpp
)
