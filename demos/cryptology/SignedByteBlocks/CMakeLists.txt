# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(signed_byte_blocks)
target_sources(signed_byte_blocks PRIVATE
        CreateKeys.cpp
        main.cpp
        SignAndVerify.cpp
        SignedByteBlocksDemos.hpp
        VerifyRecord.cpp
)
