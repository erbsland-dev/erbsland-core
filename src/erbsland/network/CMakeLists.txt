# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Host.cpp
        Host.hpp
        HostEndpoint.cpp
        HostEndpoint.hpp
        HostLookup.hpp
        HostLookupEvents.hpp
        HostName.cpp
        HostName.hpp
        IpAddress.cpp
        IpAddress.hpp
        IpEndpoint.cpp
        IpEndpoint.hpp
        IpNetwork.cpp
        IpNetwork.hpp
        IpVersion.hpp
        Network.hpp
        Network_fwd.hpp
        NetworkCallbacks.hpp
        NetworkError.hpp
        NetworkErrorContext.hpp
        NetworkSendStatus.hpp
        NetworkSourceState.hpp
        Port.cpp
        Port.hpp
        ScopeId.hpp
        SocketBufferLimits.hpp
        TcpConnection.hpp
        TcpConnectionAttempt.hpp
        TcpConnectionAttemptEvents.hpp
        TcpConnectionEvents.hpp
        TcpConnectionRequest.hpp
        TcpListener.hpp
        TcpListenerEvents.hpp
        UdpDatagram.hpp
        UdpPeer.hpp
        UdpPeerEvents.hpp
        UdpSocket.hpp
        UdpSocketEvents.hpp
)
