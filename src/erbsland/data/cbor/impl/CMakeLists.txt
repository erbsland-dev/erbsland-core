# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        CborCodec.cpp
        CborCodec.hpp
        CborCodec_write.cpp
        CborValueData.hpp
        CborValueData_fwd.hpp
)
