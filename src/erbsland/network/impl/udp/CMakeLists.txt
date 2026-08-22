# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
)

if (WIN32)
    add_subdirectory(windows)
else ()
    add_subdirectory(posix)
endif ()

target_sources(erbsland-core PRIVATE
        UdpSocket.cpp
        UdpSocket.hpp
        UdpSocket_fwd.hpp
        UdpSocketDevice.hpp
        UdpSocketDevice_fwd.hpp
        UdpSocketDeviceCallbacks.hpp
        UdpSocketDeviceCallbacks_fwd.hpp
        UdpSocketDeviceCreateFn.hpp
        UdpSocketDeviceSendStatus.hpp
        UdpSocketDeviceSendStatus_fwd.hpp
        UdpSocketEventEditor.cpp
        UdpSocketEventEditor.hpp
)
