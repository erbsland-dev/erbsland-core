# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Alignment.hpp
        AlignmentFlags.hpp
        Anchor.hpp
        AnchorFlags.hpp
        Axis.hpp
        AxisMappable.hpp
        AxisMapper.hpp
        AxisMapping.hpp
        Dimensionality.hpp
        Orientation.hpp
        SignedAxis.hpp
        SignedAxisMapping.hpp
        Symmetry.hpp
)
