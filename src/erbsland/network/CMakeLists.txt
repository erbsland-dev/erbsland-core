# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(host_lookup)
add_subdirectory(http)
add_subdirectory(http_client)
add_subdirectory(http_server)
add_subdirectory(impl)
add_subdirectory(source)
add_subdirectory(tcp)
add_subdirectory(tls)
add_subdirectory(udp)
add_subdirectory(url)

target_sources(erbsland-core PRIVATE
        Host.cpp
        Host.hpp
        HostEndpoint.cpp
        HostEndpoint.hpp
        HostName.cpp
        HostName.hpp
        HostNameFormat.hpp
        IpAddress.cpp
        IpAddress.hpp
        IpEndpoint.cpp
        IpEndpoint.hpp
        IpNetwork.cpp
        IpNetwork.hpp
        IpVersion.hpp
        Network.hpp
        Network_fwd.hpp
        Port.cpp
        Port.hpp
        ScopeId.hpp
)
