# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        FileSystemLoaderTest.cpp
        MemoryLayoutLoader.hpp
        RenderCompilerTest.cpp
        RenderContextTest.cpp
        RenderEnvironmentTest.cpp
        RenderExpressionTest.cpp
        RenderFilterTest.cpp
        RenderIncludeTest.cpp
        RenderInheritanceTest.cpp
        RenderIterationTest.cpp
        RenderLanguageCompletionTest.cpp
        RenderProgramTest.cpp
        RenderStatementTest.cpp
        RenderTokenizerTest.cpp
        RenderValueTest.cpp
        ResourceLoaderTest.cpp
)
