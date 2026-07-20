# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(builder)

target_sources(erbsland-core PRIVATE
        Constraint.hpp
        ConstraintType.cpp
        ConstraintType.hpp
        Rule.hpp
        Rules.cpp
        Rules.hpp
        RulesBuilder.cpp
        RulesBuilder.hpp
        RuleType.cpp
        RuleType.hpp
)
