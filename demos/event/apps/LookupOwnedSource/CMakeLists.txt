# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(lookup_owned_source)
target_sources(lookup_owned_source PRIVATE
        LookupOwnedSourceApp.hpp
        main.cpp
)
