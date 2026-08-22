# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TlsServerConnection.cpp
        TlsServerConnection.hpp
        TlsServerConnection_fwd.hpp
        TlsServerConnection_io.cpp
        TlsServerConnection_lifecycle.cpp
        TlsServerConnectionEventEditor.cpp
        TlsServerConnectionEventEditor.hpp
        TlsServerProtocol.cpp
        TlsServerProtocol.hpp
        TlsServerProtocol_application.cpp
        TlsServerProtocol_flight.cpp
        TlsServerProtocol_fwd.hpp
        TlsServerProtocol_handshake.cpp
        TlsServerProtocol_io.cpp
        TlsServerProtocolCheckpoint.hpp
        TlsServerProtocolOptions.cpp
        TlsServerProtocolOptions.hpp
        TlsServerProtocolState.hpp
        TlsServerProtocolTestAccess.hpp
        TlsServerProtocolTestAccess_fwd.hpp
)
