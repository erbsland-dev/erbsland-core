# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TlsClientConnection.cpp
        TlsClientConnection.hpp
        TlsClientConnection_fwd.hpp
        TlsClientConnection_io.cpp
        TlsClientConnection_lifecycle.cpp
        TlsClientConnectionEventEditor.cpp
        TlsClientConnectionEventEditor.hpp
        TlsClientHelloBuilder.cpp
        TlsClientHelloBuilder.hpp
        TlsClientProtocol.cpp
        TlsClientProtocol.hpp
        TlsClientProtocol_application.cpp
        TlsClientProtocol_fwd.hpp
        TlsClientProtocol_handshake.cpp
        TlsClientProtocol_handshake_certificates.cpp
        TlsClientProtocol_io.cpp
        TlsClientProtocolCheckpoint.hpp
        TlsClientProtocolOptions.hpp
        TlsClientProtocolStartFn.hpp
        TlsClientProtocolState.hpp
        TlsClientProtocolTestAccess.hpp
        TlsClientProtocolTestAccess_fwd.hpp
)
