# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-constant-time PRIVATE
        Ed25519Case.cpp
        Ed25519Case.hpp
        Ed25519Fixture.cpp
        Ed25519Fixture.hpp
        Ed25519Operation.hpp
)
