# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core-tool-doc PRIVATE
        ApiGuidelinesTool.cpp
        ApiGuidelinesTool.hpp
        ApiTestStatusTool.cpp
        ApiTestStatusTool.hpp
        BuildDocTool.cpp
        BuildDocTool.hpp
        CreateDemoTool.cpp
        CreateDemoTool.hpp
        DocumentationOutputFilter.cpp
        DocumentationOutputFilter.hpp
        DoxygenWarningSuppression.hpp
        PreprocessSourcesTool.cpp
        PreprocessSourcesTool.hpp
        ProcessRstTool.cpp
        ProcessRstTool.hpp
        UpdateReferenceTool.cpp
        UpdateReferenceTool.hpp
)
