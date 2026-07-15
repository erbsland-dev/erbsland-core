# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        BlockScope.hpp
        DocumentLayout.cpp
        DocumentLayout.hpp
        Helpers.cpp
        Helpers.hpp
        InlineContent.cpp
        InlineContent.hpp
        InlineTextBuilder.cpp
        InlineTextBuilder.hpp
        ListItemLayout.cpp
        ListItemLayout.hpp
        RenderBlock.cpp
        RenderBlock.hpp
        RenderContext.hpp
        RenderEngine.cpp
        RenderEngine.hpp
        RenderEngine_inline.cpp
        RenderEngine_lists.cpp
        Renderer.cpp
        Renderer.hpp
)
