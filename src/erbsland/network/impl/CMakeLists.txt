# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(host)
add_subdirectory(http)
add_subdirectory(platform)
add_subdirectory(tcp)
add_subdirectory(tls)
add_subdirectory(udp)
add_subdirectory(url)

target_sources(erbsland-core PRIVATE
        ConnectionProtocolAccess.hpp
        ConnectionProtocolAccess_fwd.hpp
        NetworkBackend.cpp
        NetworkBackend.hpp
        NetworkErrorDiagnostic.cpp
        NetworkErrorDiagnostic.hpp
        ResolverService.cpp
        ResolverService.hpp
)
