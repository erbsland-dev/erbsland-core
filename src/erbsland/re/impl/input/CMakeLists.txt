# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        NullRejectingInput.cpp
        NullRejectingInput.hpp
        StreamInput.cpp
        StreamInput.hpp
        StreamMatch.cpp
        StreamMatch.hpp
        StringInput.cpp
        StringInput.hpp
        StringMatch.cpp
        StringMatch.hpp
        U16StringInput.cpp
        U16StringInput.hpp
        U16StringMatch.cpp
        U16StringMatch.hpp
        U32StringInput.cpp
        U32StringInput.hpp
        U32StringMatch.cpp
        U32StringMatch.hpp
)
