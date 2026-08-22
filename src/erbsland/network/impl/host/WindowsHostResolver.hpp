// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostResolver.hpp"

#include "../platform/WindowsNetworkRuntime.hpp"

#include "../../../core/impl/WindowsApi.hpp"
#include "../../../system/PlatformErrorCategory.hpp"

#include <winsock2.h>

#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// Windows implementation of the native host resolver.
/// @tested{HostResolverTest}
class WindowsHostResolver final : public HostResolver {
public:
    /// Create a Windows host resolver.
    WindowsHostResolver();

public: // implement HostResolver
    [[nodiscard]] auto resolve(const HostName &hostName) -> util::List<IpAddress> override;

private:
    /// Convert a native socket address to an IP address.
    [[nodiscard]] static auto addressFrom(const sockaddr *socketAddress, int family) noexcept
        -> std::optional<IpAddress>;
    /// Convert a Winsock error code to a platform error category.
    [[nodiscard]] static auto errorCategory(int errorCode) noexcept -> system::PlatformErrorCategory;
    /// Test whether a Winsock error can be retried.
    [[nodiscard]] static auto isTemporaryError(int errorCode) noexcept -> bool;
    /// Format a Winsock error message.
    [[nodiscard]] static auto errorMessage(int errorCode) -> text::String;
    std::shared_ptr<WindowsNetworkRuntime> _runtime; ///< Shared Winsock lifetime.
};

}
