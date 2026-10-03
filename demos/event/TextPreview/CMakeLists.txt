# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(text_preview)
target_sources(text_preview PRIVATE
        main.cpp
        TextPreviewApp.cpp
        TextPreviewApp.hpp
)
