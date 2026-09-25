# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(render_layouts)
target_sources(render_layouts PRIVATE
        ContextValues.cpp
        CustomLoader.cpp
        EnvironmentExamples.cpp
        FeatureCheatSheet.cpp
        Introduction.cpp
        LanguageConditions.cpp
        LanguageExpressions.cpp
        LanguageIterations.cpp
        LayoutInheritance.cpp
        LazyValue.cpp
        LoaderExamples.cpp
        main.cpp
        RenderLayoutsDemos.hpp
        RenderWorkflow.cpp
        ValueShapes.cpp
)

erbsland_core_add_resources(
        TARGET render_layouts
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "render-layouts"
        RECURSIVE
        SUFFIXES ".txt"
)
