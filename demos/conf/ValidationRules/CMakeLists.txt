# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(validation_rules)
target_sources(validation_rules PRIVATE
        BuiltRules.cpp
        EmbeddedRules.cpp
        main.cpp
        ValidationRulesDemos.hpp
)
erbsland_core_add_resources(
        TARGET validation_rules
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/data"
        IDENTIFIER "conf-validation-rules"
        SUFFIXES ".elcl"
)
