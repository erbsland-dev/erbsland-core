# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        BlockRole.hpp
        BlockStringWithMargins.cpp
        BlockStringWithMargins.hpp
        Element.hpp
        Identifier.hpp
        LayoutHelper.cpp
        LayoutHelper.hpp
        LayoutRectangle.hpp
        Part.hpp
        Properties.cpp
        Properties.hpp
        PropertyEditor.cpp
        PropertyEditor.hpp
        PropertySheet.cpp
        PropertySheet.hpp
        Selector.cpp
        Selector.hpp
        State.hpp
        States.cpp
        States.hpp
        Tag.hpp
        Tags.hpp
        Theme.cpp
        Theme.hpp
        ThemeAccessor.cpp
        ThemeAccessor.hpp
        ThemeBuilder.cpp
        ThemeBuilder.hpp
        ThemePainter.cpp
        ThemePainter.hpp
)
