# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core-tool-core PRIVATE
        CategoryTestDataTool.cpp
        CategoryTestDataTool.hpp
        CryptologyVectorsTool.cpp
        CryptologyVectorsTool.hpp
        CryptologyVectorsTool_execute.cpp
        FixtureTool.cpp
        FixtureTool.hpp
        SaturatingMathTool.cpp
        SaturatingMathTool.hpp
        TimeTestsTool.cpp
        TimeTestsTool.hpp
)
