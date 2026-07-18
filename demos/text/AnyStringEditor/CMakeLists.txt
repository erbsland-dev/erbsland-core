# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(any_string_editor)
target_sources(any_string_editor PRIVATE
        AcceptAny.cpp
        AnyStringEditorDemos.hpp
        main.cpp
)
