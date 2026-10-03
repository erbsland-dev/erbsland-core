# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core-resource-integration PRIVATE
        main.cpp
)

target_sources(erbsland-core-resource-static-link PRIVATE
        empty.cpp
)
