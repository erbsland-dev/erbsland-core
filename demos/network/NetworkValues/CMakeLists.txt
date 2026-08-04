# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(network_values)
target_sources(network_values PRIVATE
        BuildEndpoints.cpp
        MatchNetworks.cpp
        NetworkValuesDemos.hpp
        ParseAddressesAndHosts.cpp
        main.cpp
)
