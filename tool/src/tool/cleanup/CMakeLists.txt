# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        ClangFormat.cpp
        ClangFormat.hpp
        CleanupConfiguration.hpp
        CleanupPolicy.cpp
        CleanupPolicy.hpp
        CleanupResult.hpp
        CleanupText.cpp
        CleanupText.hpp
        CMakeCleanup.cpp
        CMakeCleanup.hpp
        CodeCleanup.cpp
        CodeCleanup.hpp
        CodeCleanupTool.cpp
        CodeCleanupTool.hpp
        CppCleanup.cpp
        CppCleanup.hpp
        CppSpacing.cpp
        CppSpacing.hpp
        IncludeIndex.cpp
        IncludeIndex.hpp
        SourceTree.hpp
)
