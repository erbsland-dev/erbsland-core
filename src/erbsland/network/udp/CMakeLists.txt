# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        UdpDatagram.hpp
        UdpDatagramDropContext.hpp
        UdpDatagramDropContext_fwd.hpp
        UdpDatagramDropFn.hpp
        UdpDatagramDropReason.hpp
        UdpDatagramDropReason_fwd.hpp
        UdpDatagramFn.hpp
        UdpSocket.cpp
        UdpSocket.hpp
        UdpSocket_fwd.hpp
        UdpSocketEventEditor.hpp
        UdpSocketEventEditor_fwd.hpp
        UdpSocketOptions.hpp
        UdpSocketOptions_fwd.hpp
)
