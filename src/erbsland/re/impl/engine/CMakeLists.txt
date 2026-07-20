# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CaptureGroupManager.cpp
        CaptureGroupManager.hpp
        CaptureGroupManagerBase.hpp
        CaptureGroupManagerWithAtomic.hpp
        CaptureGroupNames.hpp
        CaptureGroupSet.hpp
        CaptureGroupSetWithAtomic.hpp
        CaptureGroupTypes.hpp
        CharClassData.hpp
        Counter.hpp
        Engine.cpp
        Engine.hpp
        Engine_char.cpp
        Engine_flow.cpp
        Engine_fwd.hpp
        EngineCharacterState.hpp
        EngineData.hpp
        EngineDebug.hpp
        EngineFlags.hpp
        EngineMatch.hpp
        EngineResult.hpp
        EngineState.hpp
        EngineThread.hpp
        EngineWork.hpp
        Operation.hpp
        Program.hpp
        ProgramHelpers.hpp
        ProgramReader.hpp
        ProgramWriter.hpp
        SequenceData.hpp
        Watchdog.hpp
)
