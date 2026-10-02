# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-constant-time PRIVATE
        RsaCase.cpp
        RsaCase.hpp
        RsaContext.cpp
        RsaContext.hpp
        RsaFixture.cpp
        RsaFixture.hpp
        RsaOperation.hpp
)
