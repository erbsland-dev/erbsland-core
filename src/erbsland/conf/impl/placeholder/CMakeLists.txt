# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        EnvironmentPlaceholderSource.cpp
        EnvironmentPlaceholderSource.hpp
        PlaceholderResolver.cpp
        PlaceholderResolver.hpp
        PlaceholderResolver_fwd.hpp
        PlaceholderTextParser.cpp
        PlaceholderTextParser.hpp
        TextPlaceholderFilter.cpp
        TextPlaceholderFilter.hpp
        TextPlaceholderParameterKey.hpp
        TextPlaceholderParameters.cpp
        TextPlaceholderParameters.hpp
        VariablePlaceholderSource.cpp
        VariablePlaceholderSource.hpp
)
