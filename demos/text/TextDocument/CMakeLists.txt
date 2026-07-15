# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(text_document)
target_sources(text_document PRIVATE
        main.cpp
        RenderCodeSnippet.cpp
        TextDocumentDemos.hpp
)
