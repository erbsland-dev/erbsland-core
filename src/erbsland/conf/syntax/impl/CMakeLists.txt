# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        LayoutLine.hpp
        NamePathPattern.cpp
        NamePathPattern.hpp
        Reader.cpp
        Reader.hpp
        Reader_fwd.hpp
        SnapshotSource.hpp
        Writer.cpp
        Writer.hpp
        Writer_values.cpp
        Writer_vectors.cpp
        Writer_wrapping.cpp
)
