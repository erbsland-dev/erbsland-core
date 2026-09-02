# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        CoordinateSpanTest.cpp
        CoordinateTest.cpp
        DirectionTest.cpp
        MarginPairTest.cpp
        MarginsTest.cpp
        PositionTest.cpp
        RectangleTest.cpp
        SizeTest.cpp
        StdFormatForBlockTest.cpp
)
