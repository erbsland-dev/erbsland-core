# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(tcp_map_client)
target_sources(tcp_map_client PRIVATE
        TcpMapClientApp.hpp
        main.cpp
)
