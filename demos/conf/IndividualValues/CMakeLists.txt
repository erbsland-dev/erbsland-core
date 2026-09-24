# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(individual_values)
target_sources(individual_values PRIVATE
        IndividualValuesDemos.hpp
        main.cpp
        ScalarConversions.cpp
        TypeTests.cpp
        TypedCollections.cpp
        TypedGetters.cpp
        ValueLists.cpp
)
