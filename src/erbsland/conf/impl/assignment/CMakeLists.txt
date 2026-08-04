# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Assignment.hpp
        AssignmentGenerator.hpp
        AssignmentStream.cpp
        AssignmentStream.hpp
        AssignmentStream_fwd.hpp
        AssignmentType.hpp
)
