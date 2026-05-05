# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(random_topics)
target_sources(random_topics PRIVATE
        ApiOverview.cpp
        ElementSampling.cpp
        main.cpp
        RandomTopicsDemos.hpp
        RandomValues.cpp
        SecureTokens.cpp
)
