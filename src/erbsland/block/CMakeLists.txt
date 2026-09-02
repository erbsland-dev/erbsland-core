# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        AlignedSource.hpp
        AlignedSource_fwd.hpp
        Coordinate.hpp
        CoordinateSpan.hpp
        Direction.cpp
        Direction.hpp
        MarginPair.hpp
        Margins.hpp
        Position.cpp
        Position.hpp
        Position.tpp
        Position_fwd.hpp
        PositionList.hpp
        PositionList_fwd.hpp
        Rectangle.cpp
        Rectangle.hpp
        Rectangle.tpp
        Rectangle_fwd.hpp
        RectangleList.hpp
        RectangleList_fwd.hpp
        Size.cpp
        Size.hpp
        Size.tpp
        Size_fwd.hpp
        StdFormat.hpp
)
