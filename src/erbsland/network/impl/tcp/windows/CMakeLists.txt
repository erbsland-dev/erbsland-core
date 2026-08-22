# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        WindowsTcpAcceptOperation.hpp
        WindowsTcpConnectionDevice.cpp
        WindowsTcpConnectionDevice.hpp
        WindowsTcpConnectionOperation.hpp
        WindowsTcpConnectionState.hpp
        WindowsTcpListenerDevice.cpp
        WindowsTcpListenerDevice.hpp
        WindowsTcpListenerState.hpp
        WindowsTcpSocket.cpp
        WindowsTcpSocket.hpp
)
