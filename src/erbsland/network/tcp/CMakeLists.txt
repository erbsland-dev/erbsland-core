# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        TcpAcceptOptions.hpp
        TcpAcceptOptions_fwd.hpp
        TcpConnection.cpp
        TcpConnection.hpp
        TcpConnection_fwd.hpp
        TcpConnectionEventEditor.hpp
        TcpConnectionEventEditor_fwd.hpp
        TcpConnectionFilterFn.hpp
        TcpConnectionFilterResult.hpp
        TcpConnectionFilterResult_fwd.hpp
        TcpConnectionFn.hpp
        TcpConnectionRequest.hpp
        TcpConnectionRequest_fwd.hpp
        TcpConnectionRequestFn.hpp
        TcpConnectionRequestState.hpp
        TcpConnectionRequestState_fwd.hpp
        TcpConnectOptions.hpp
        TcpConnectOptions_fwd.hpp
        TcpHostResolvedFn.hpp
        TcpListener.cpp
        TcpListener.hpp
        TcpListener_fwd.hpp
        TcpListenerEventEditor.hpp
        TcpListenerEventEditor_fwd.hpp
        TcpListenerOptions.cpp
        TcpListenerOptions.hpp
        TcpListenerOptions_fwd.hpp
)
