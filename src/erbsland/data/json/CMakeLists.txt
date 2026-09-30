# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(impl)

target_sources(erbsland-core PRIVATE
        JsonContainers.hpp
        JsonFormatOptions.hpp
        JsonParseOptions.hpp
        JsonType.hpp
        JsonValue.cpp
        JsonValue.hpp
        JsonValue_fwd.hpp
        JsonValue_get.tpp
)
