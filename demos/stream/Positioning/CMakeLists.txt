# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(positioning)
target_sources(positioning PRIVATE
        CheckPositioning.cpp
        CreateSparseRecord.cpp
        HandlePositionTimeout.cpp
        main.cpp
        MoveRelativeToEnd.cpp
        PositionEncodedText.cpp
        PositioningDemos.hpp
        ReadRecordAtOffset.cpp
)
