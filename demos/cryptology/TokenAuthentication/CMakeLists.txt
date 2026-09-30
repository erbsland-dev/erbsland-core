# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(token_authentication)
target_sources(token_authentication PRIVATE
        Client.hpp
        JsonLine.hpp
        Server.hpp
        main.cpp
)
