// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "WindowsUdpSocketOperation_fwd.hpp"

#include "../../../../mem/impl/UnsafeByteBlockBuffer.hpp"
#include "../../platform/SocketAddress.hpp"

#include <cstdint>

namespace erbsland::network::impl {

/// Stable storage for one Windows overlapped UDP operation.
/// @tested{UdpSocketLiveTest}
struct WindowsUdpSocketOperation final {
    /// Create an operation record.
    /// @param operationGeneration The owning socket generation.
    explicit WindowsUdpSocketOperation(const std::uint64_t operationGeneration) noexcept :
        generation{operationGeneration} {}

    OVERLAPPED overlapped{};                                           ///< Native overlapped operation record.
    std::uint64_t generation;                                          ///< Owning socket generation.
    WSABUF nativeBuffer{};                                             ///< Native buffer descriptor.
    DWORD flags{};                                                     ///< Native receive flags.
    SocketAddress remoteAddress;                                       ///< Remote endpoint storage.
    int remoteAddressSize{static_cast<int>(sizeof(sockaddr_storage))}; ///< Remote endpoint size.
    mem::impl::UnsafeByteBlockBuffer receiveBuffer;                    ///< Owned receive buffer.
};

}
