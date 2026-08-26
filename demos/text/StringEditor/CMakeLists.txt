# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(string_editor)
target_sources(string_editor PRIVATE
        AlignAndTruncate.cpp
        ClearResetAndAppend.cpp
        CopyOnWrite.cpp
        EditingText.cpp
        InsertAndReplace.cpp
        ManualDetach.cpp
        main.cpp
        NormalizeText.cpp
        RemoveAndKeepRanges.cpp
        RemoveFirstAndAll.cpp
        ReserveForAppend.cpp
        ShrinkSlices.cpp
        StringEditorDemos.hpp
)
