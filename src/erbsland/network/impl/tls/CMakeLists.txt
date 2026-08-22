# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

add_subdirectory(client)
add_subdirectory(server)

target_sources(erbsland-core PRIVATE
        TlsAlertDescription.hpp
        TlsAlpnProtocol.hpp
        TlsHandshakeStream.cpp
        TlsHandshakeStream.hpp
        TlsProtocolError.cpp
        TlsProtocolError.hpp
        TlsRecordStream.cpp
        TlsRecordStream.hpp
        TlsWireReader.cpp
        TlsWireReader.hpp
        TlsWireWriter.cpp
        TlsWireWriter.hpp
)
