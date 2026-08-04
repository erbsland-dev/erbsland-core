// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpConnectionOperation.hpp is only available on Windows."
#endif

#include "../../core/impl/WindowsApi.hpp"
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"

// The Windows SDK requires winsock2.h before mswsock.h.
// clang-format off
#include <winsock2.h>
// clang-format on

#include <cstdint>

namespace erbsland::network::impl {

/// Stable record for one overlapped Windows TCP operation.
/// @tested{TcpSocketLiveTest}
struct WindowsTcpConnectionOperation final {
    /// Identify the native TCP operation represented by this record.
    enum class Type : std::uint8_t {
        Connect, ///< Establish the TCP connection.
        Receive, ///< Receive incoming data.
        Send,    ///< Send queued data.
    };

    /// Create an operation record with its generation.
    explicit WindowsTcpConnectionOperation(Type operationType, std::uint64_t operationGeneration) noexcept :
        type{operationType}, generation{operationGeneration} {}

    OVERLAPPED overlapped{};                        ///< Native overlapped record.
    Type type;                                      ///< Operation kind.
    std::uint64_t generation;                       ///< Owning state generation.
    WSABUF nativeBuffer{};                          ///< Native buffer descriptor.
    DWORD flags{};                                  ///< Native receive flags.
    mem::impl::UnsafeByteBlockBuffer receiveBuffer; ///< Owned receive storage.
    mem::ByteBlock sendData;                        ///< Retained send storage.
    std::size_t sendOffset{};                       ///< Bytes already sent.
};

}
