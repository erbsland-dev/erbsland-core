# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)
add_subdirectory(rules)

target_sources(erbsland-core-tool-lib PRIVATE
        AntiPatternsTool.cpp
        AntiPatternsTool.hpp
        ExceptionFromRule.hpp
        Finding.hpp
        FindingCache.cpp
        FindingCache.hpp
        Policy.cpp
        Policy.hpp
        Report.cpp
        Report.hpp
        Rule.hpp
        RuleRegistry.cpp
        RuleRegistry.hpp
        Scanner.cpp
        Scanner.hpp
        Severity.hpp
        SourceBlock.hpp
        SourceIndex.cpp
        SourceIndex.hpp
)
