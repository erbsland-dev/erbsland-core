// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#ifndef _WIN32
#error "WindowsTcpAcceptOperation.hpp is only available on Windows."
#endif

#include "../../core/impl/WindowsApi.hpp"

// The Windows SDK requires winsock2.h before mswsock.h.
// clang-format off
#include <winsock2.h>
// clang-format on

#include <array>
#include <cstdint>

namespace erbsland::network::impl {

/// Stable record for one overlapped AcceptEx operation.
/// @tested{TcpSocketLiveTest}
struct WindowsTcpAcceptOperation final {
    /// Creates an accept-operation record.
    /// @param operationGeneration The owning listener generation.
    explicit WindowsTcpAcceptOperation(std::uint64_t operationGeneration) noexcept : generation{operationGeneration} {}
    /// Releases the socket associated with the operation.
    ~WindowsTcpAcceptOperation();

    OVERLAPPED overlapped{};                                                      ///< Native overlapped record.
    SOCKET socket{INVALID_SOCKET};                                                ///< Socket populated by AcceptEx.
    std::uint64_t generation;                                                     ///< Owning listener generation.
    std::array<std::byte, 2U * (sizeof(sockaddr_storage) + 16U)> addressBuffer{}; ///< Address workspace.
};

}
