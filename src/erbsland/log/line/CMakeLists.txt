# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        LogLine.cpp
        LogLine.hpp
        LogLine_fwd.hpp
        LogLinePart.cpp
        LogLinePart.hpp
        LogLineSegment.hpp
)
