// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostResolver.hpp"

#include "../../system/PlatformErrorCategory.hpp"

#include <netdb.h>
#include <sys/socket.h>

#include <optional>

namespace erbsland::network::impl {

/// POSIX implementation of the native host resolver.
/// @tested{HostResolverTest}
class PosixHostResolver final : public HostResolver {
public:
    [[nodiscard]] auto resolve(const HostName &hostName) -> util::List<IpAddress> override;

private:
    /// Convert a native socket address into an IP address.
    [[nodiscard]] static auto addressFrom(const sockaddr *socketAddress, int family) noexcept
        -> std::optional<IpAddress>;
    /// Map a native resolver error to a platform error category.
    [[nodiscard]] static auto errorCategory(int errorCode) noexcept -> system::PlatformErrorCategory;
    /// Test whether a native resolver error is temporary.
    [[nodiscard]] static auto isTemporaryError(int errorCode) noexcept -> bool;
};

}
