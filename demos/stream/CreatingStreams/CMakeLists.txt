# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(creating_streams)
target_sources(creating_streams PRIVATE
        ChooseStreamFamily.cpp
        ConfigureAtCreation.cpp
        CreatingStreamsDemos.hpp
        main.cpp
        OpenFileStreams.cpp
        ShareStreamWithWriter.cpp
)
