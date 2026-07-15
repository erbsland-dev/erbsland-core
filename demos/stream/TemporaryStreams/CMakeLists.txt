# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(temporary_streams)
target_sources(temporary_streams PRIVATE
        CleanupAfterAbort.cpp
        CreateTemporaryBytes.cpp
        CreateTemporaryText.cpp
        main.cpp
        ReleaseTemporaryFile.cpp
        RemoveOnClose.cpp
        TemporaryStreamsDemos.hpp
)
