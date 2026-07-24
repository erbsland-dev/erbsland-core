# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CaptureGroupManagerTest.cpp
        EngineAnchorLineStartEndTest.cpp
        EngineAnchorStartEndTextTest.cpp
        EngineAnchorWordBoundaryUnicodeAsciiTest.cpp
        EngineAnyTest.cpp
        EngineBase.hpp
        EngineBasicTest.cpp
        EngineCaptureTest.cpp
        EngineCategoryTest.cpp
        EngineCharTest.cpp
        EngineClassTest.cpp
        EngineCounterMaximumMinimumTest.cpp
        EngineFindFirstTest.cpp
        EngineJumpTest.cpp
        EngineMatchVsFullMatchTest.cpp
        EngineSequenceTest.cpp
        EngineSkipIfMaximumTest.cpp
        EngineSplitTest.cpp
        MockStringInput.hpp
        MockStringMatch.hpp
        OperationTest.cpp
        ProgramReaderWriterTest.cpp
        ProgramTest.cpp
)
