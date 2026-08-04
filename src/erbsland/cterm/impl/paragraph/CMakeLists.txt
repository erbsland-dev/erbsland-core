# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Layout.cpp
        Layout.hpp
        LayoutContext.hpp
        LayoutFragment.hpp
        LayoutFragments.hpp
        LayoutLine.hpp
        LayoutLineToken.cpp
        LayoutLineToken.hpp
        LayoutNewlineMode.hpp
        LayoutPreparedSourceLine.hpp
        LayoutResult.cpp
        LayoutResult.hpp
        LayoutSemantics.cpp
        LayoutSemantics.hpp
        LayoutSpacingRun.hpp
        LineBuilder.cpp
        LineBuilder.hpp
        Painter.cpp
        Painter.hpp
        Printer.cpp
        Printer.hpp
        RendererBase.cpp
        RendererBase.hpp
)
