# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-doc PRIVATE
        ApiGuidelines.cpp
        ApiGuidelines.hpp
        ApiGuidelineState.cpp
        ApiGuidelineState.hpp
        ApiTestStatus.cpp
        ApiTestStatus.hpp
        DemoCapture.cpp
        DemoCapture.hpp
        DemoCreation.cpp
        DemoCreation.hpp
        DemoExecutor.cpp
        DemoExecutor.hpp
        DemoFingerprint.cpp
        DemoFingerprint.hpp
        DemoRunOutput.hpp
        DemoSynchronizer.cpp
        DemoSynchronizer.hpp
        DoxygenIndex.cpp
        DoxygenIndex.hpp
        DoxygenSymbol.hpp
        HeaderScanner.cpp
        HeaderScanner.hpp
        ReferenceDocumentation.cpp
        ReferenceDocumentation.hpp
        ReferenceGroup.hpp
        RstCleaner.cpp
        RstCleaner.hpp
        SourcePreprocessor.cpp
        SourcePreprocessor.hpp
        SourceSelectors.cpp
        SourceSelectors.hpp
)
