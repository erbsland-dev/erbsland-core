# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(config_check PRIVATE
        ConfigCheckApp.cpp
        ConfigCheckApp.hpp
        main.cpp
)
target_sources(erbsland-core-config-check-support PRIVATE
        ConfigurationBatch.cpp
        ConfigurationBatch.hpp
)
