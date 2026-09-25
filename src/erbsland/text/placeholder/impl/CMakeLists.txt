# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Name.cpp
        Name.hpp
        Registry.cpp
        Registry.hpp
        Registry_fwd.hpp
        TextParameterKey.hpp
        TextParameters.cpp
        TextParameters.hpp
)
