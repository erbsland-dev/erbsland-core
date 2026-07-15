# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(working_with_paths)
target_sources(working_with_paths PRIVATE
        ConstructionAndFormats.cpp
        ConvertingPaths.cpp
        EditingPathNames.cpp
        InspectingPaths.cpp
        JoiningAndSlicing.cpp
        main.cpp
        WorkingWithPathsDemos.hpp
)
