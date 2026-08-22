# Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28)

target_sources(erbsland-core PRIVATE
        Connection.hpp
        Connection_fwd.hpp
        ConnectionCloseContext.hpp
        ConnectionCloseContext_fwd.hpp
        ConnectionCloseFn.hpp
        ConnectionCloseOrigin.hpp
        ConnectionCloseOrigin_fwd.hpp
        ConnectionEventEditor.hpp
        ConnectionEventEditor_fwd.hpp
        ConnectionQuota.cpp
        ConnectionQuota.hpp
        ConnectionQuota_fwd.hpp
        ConnectionQuotaLease.cpp
        ConnectionQuotaLease.hpp
        ConnectionQuotaLease_fwd.hpp
        ConnectionState.hpp
        NetworkDataFn.hpp
        NetworkError.cpp
        NetworkError.hpp
        NetworkErrorContext.hpp
        NetworkErrorContext_fwd.hpp
        NetworkErrorFn.hpp
        NetworkErrorPhase.hpp
        NetworkErrorReason.hpp
        NetworkErrorReason_fwd.hpp
        NetworkEventFn.hpp
        NetworkSendStatus.hpp
        NetworkSourceState.hpp
        SocketBufferLimits.hpp
)
