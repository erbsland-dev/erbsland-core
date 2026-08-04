# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

erbsland_core_add_demo(udp_sockets)
target_sources(udp_sockets PRIVATE
        PauseDelivery.cpp
        UdpSocketsDemos.hpp
        main.cpp
)
