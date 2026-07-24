# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Alignment.hpp
        AlignmentFlags.hpp
        BlockAlignedSource.hpp
        BlockAlignedSource_fwd.hpp
        BlockAnchor.hpp
        BlockAxisMapper.hpp
        BlockCoordinate.hpp
        BlockDirection.cpp
        BlockDirection.hpp
        BlockMargins.hpp
        BlockPosition.cpp
        BlockPosition.hpp
        BlockPosition.tpp
        BlockPosition_fwd.hpp
        BlockPositionList.hpp
        BlockPositionList_fwd.hpp
        BlockRectangle.cpp
        BlockRectangle.hpp
        BlockRectangle.tpp
        BlockRectangle_fwd.hpp
        BlockRectangleList.hpp
        BlockRectangleList_fwd.hpp
        BlockSize.cpp
        BlockSize.hpp
        BlockSize_fwd.hpp
        Orientation.hpp
        StdFormat.hpp
        Symmetry.hpp
)
