# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-unittest PRIVATE
        BlockStringConversionTest.cpp
        BlockStringEditorTest.cpp
        BlockStringTest.cpp
        BlockTest.cpp
        BlockTextOptionsTest.cpp
        BlockTextRenderTest.cpp
        BlockTextTest.cpp
        FontAccessTest.cpp
        FontTest.cpp
        ParagraphIndentsTest.cpp
        ParagraphOptionsTest.cpp
        ParagraphRenderTest.cpp
        TerminalDocumentRendererTest.cpp
        TerminalDocumentStyleTest.cpp
)
