# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        DisplayText.cpp
        DisplayText.hpp
        DisplayTextCore.cpp
        DisplayTextCore.hpp
        DisplayTextDomain.hpp
        DisplayTextDomainEntry.hpp
)
