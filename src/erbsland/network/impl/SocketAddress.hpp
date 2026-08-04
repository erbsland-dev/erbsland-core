// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IpEndpoint.hpp"

#include "../../core/Definitions.hpp"

#ifdef ERBSLAND_OS_WINDOWS
#include "../../core/impl/WindowsApi.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <optional>

namespace erbsland::network::impl {

/// A native IPv4 or IPv6 socket address with portable conversion.
/// @tested{UdpSocketLiveTest}
class SocketAddress final {
public:
    /// Create an empty native socket address.
    SocketAddress() = default;

public: // accessors
    /// Access the native address for read-only APIs.
    [[nodiscard]] auto data() const noexcept -> const sockaddr *;
    /// Access the native address for APIs that fill it.
    [[nodiscard]] auto data() noexcept -> sockaddr *;
    /// Get the current native address size.
    [[nodiscard]] auto size() const noexcept -> int { return _size; }
    /// Access the native address-size storage for APIs that fill it.
    [[nodiscard]] auto sizePointer() noexcept -> int * { return &_size; }
    /// Get the native address family.
    [[nodiscard]] auto family() const noexcept -> int;

public: // conversion
    /// Convert the native address to a portable endpoint.
    /// @return The endpoint, or `std::nullopt` for an unsupported family.
    [[nodiscard]] auto toEndpoint() const noexcept -> std::optional<IpEndpoint>;
    /// Convert a portable endpoint to a native socket address.
    /// @param endpoint The endpoint to convert.
    /// @return The native socket address.
    [[nodiscard]] static auto fromEndpoint(const IpEndpoint &endpoint) noexcept -> SocketAddress;

private:
    sockaddr_storage _storage{};                   ///< Native address storage.
    int _size{static_cast<int>(sizeof(_storage))}; ///< Current native address size.
};

}
