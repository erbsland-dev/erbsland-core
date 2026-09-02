# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(attribute)
add_subdirectory(constraint)

target_sources(erbsland-core PRIVATE
        Attributes.hpp
        Constraints.hpp
        RuleDefinition.hpp
)
