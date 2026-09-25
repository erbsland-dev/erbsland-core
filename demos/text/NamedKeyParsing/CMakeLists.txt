# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(named_key_parsing)
target_sources(named_key_parsing PRIVATE
        ChooseEntryPolicies.cpp
        ChooseSeparators.cpp
        LimitAnswerValues.cpp
        main.cpp
        NamedKeyParsingDemos.hpp
        ParseFactRequest.cpp
        PrefixAndCompactValues.cpp
        RegisterQuizKeys.cpp
        RestrictValueCharacters.cpp
)
