// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostResolver_fwd.hpp"

#include "../HostName.hpp"
#include "../IpAddress.hpp"

#include "../../util/List.hpp"

namespace erbsland::network::impl {

/// Backend interface for blocking native host resolution.
/// @tested{HostLookupTest HostResolverTest}
class HostResolver {
public:
    // defaults
    virtual ~HostResolver() = default;

public:
    /// Resolve a host name to unique IPv4 and IPv6 addresses.
    /// @param hostName The opaque name passed to the platform resolver.
    /// @return Addresses in platform resolver order.
    /// @throws system::PlatformError If the native resolver fails.
    [[nodiscard]] virtual auto resolve(const HostName &hostName) -> util::List<IpAddress> = 0;
};

/// Create the resolver for the current platform.
/// @return The native resolver.
/// @tested{HostResolverTest}
[[nodiscard]] auto createHostResolver() -> HostResolverPtr;

}
