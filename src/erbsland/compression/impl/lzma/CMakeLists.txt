# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        LzmaCodec.cpp
        LzmaCodec.hpp
        LzmaDecoder.cpp
        LzmaDecoder.hpp
        LzmaEncoder.cpp
        LzmaEncoder.hpp
        LzmaLengthProbabilities.hpp
        LzmaProbabilities.cpp
        LzmaProbabilities.hpp
        LzmaRangeDecoder.cpp
        LzmaRangeDecoder.hpp
        LzmaRangeEncoder.cpp
        LzmaRangeEncoder.hpp
)
