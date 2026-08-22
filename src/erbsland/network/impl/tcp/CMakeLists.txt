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
        TcpAcceptedSocket.hpp
        TcpAcceptedSocket_fwd.hpp
        TcpConnection.cpp
        TcpConnection.hpp
        TcpConnection_fwd.hpp
        TcpConnection_io.cpp
        TcpConnectionDevice.hpp
        TcpConnectionDevice_fwd.hpp
        TcpConnectionDeviceCallbacks.hpp
        TcpConnectionDeviceCallbacks_fwd.hpp
        TcpConnectionDeviceCreateFn.hpp
        TcpConnectionDeviceSendStatus.hpp
        TcpConnectionDeviceSendStatus_fwd.hpp
        TcpConnectionEventEditor.cpp
        TcpConnectionEventEditor.hpp
        TcpConnectionRequest.cpp
        TcpConnectionRequest.hpp
        TcpConnectionRequest_fwd.hpp
        TcpListener.cpp
        TcpListener.hpp
        TcpListener_fwd.hpp
        TcpListenerDevice.hpp
        TcpListenerDevice_fwd.hpp
        TcpListenerDeviceCallbacks.hpp
        TcpListenerDeviceCallbacks_fwd.hpp
        TcpListenerDeviceCreateFn.hpp
        TcpListenerEventEditor.cpp
        TcpListenerEventEditor.hpp
)
