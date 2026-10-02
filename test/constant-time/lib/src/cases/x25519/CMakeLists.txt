# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-constant-time PRIVATE
        X25519Case.cpp
        X25519Case.hpp
        X25519Fixture.cpp
        X25519Fixture.hpp
        X25519Operation.hpp
)
