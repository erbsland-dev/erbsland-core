# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-tool-lib PRIVATE
        NativeGitFile.cpp
        NativeGitFile.hpp
        TaskDisplay.cpp
        TaskDisplay.hpp
        TaskLogWriter.cpp
        TaskLogWriter.hpp
)
