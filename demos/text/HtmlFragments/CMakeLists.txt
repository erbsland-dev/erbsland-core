# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(html_fragments)
target_sources(html_fragments PRIVATE
        HtmlFragmentsDemos.hpp
        main.cpp
        RenderFragment.cpp
)
