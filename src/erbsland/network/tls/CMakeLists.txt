# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TlsAlertDescription.hpp
        TlsClientConnection.cpp
        TlsClientConnection.hpp
        TlsClientConnection_fwd.hpp
        TlsClientConnectionEventEditor.hpp
        TlsClientConnectionEventEditor_fwd.hpp
        TlsClientConnectOptions.hpp
        TlsClientConnectOptions_fwd.hpp
        TlsServerAcceptOptions.hpp
        TlsServerAcceptOptions_fwd.hpp
        TlsServerConnection.hpp
        TlsServerConnection_fwd.hpp
        TlsServerConnectionEventEditor.hpp
        TlsServerConnectionEventEditor_fwd.hpp
        TlsServerIdentityMapping.hpp
)
