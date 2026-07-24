# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(hash_algorithms)
target_sources(hash_algorithms PRIVATE
        FixedSha3.cpp
        HashAlgorithmsDemos.hpp
        HashBoundedInput.cpp
        main.cpp
        PersistAlgorithm.cpp
        SelectAlgorithm.cpp
        StoreDigest.cpp
)
