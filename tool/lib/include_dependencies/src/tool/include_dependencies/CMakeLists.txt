# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-include_dependencies PRIVATE
        DependencyAnalysis.hpp
        DependencyRule.hpp
        IncludeDependenciesTool.cpp
        IncludeDependenciesTool.hpp
        IncludeEdge.hpp
        IncludeGraph.cpp
        IncludeGraph.hpp
        IncludeGraph_reports.cpp
        IncludeNode.hpp
)
