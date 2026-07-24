// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostEndpoint.hpp"
#include "IpEndpoint.hpp"

#include "../system/PlatformErrorContext_fwd.hpp"
#include "../text/String.hpp"

#include <optional>

namespace erbsland::network {

/// Structured context for an asynchronous network failure.
/// @tested{NetworkFacadeTest}
class NetworkErrorContext final {
public:
    /// Create a network error context.
    /// @param title The short error title.
    /// @param description The user-facing error description.
    NetworkErrorContext(text::String title, text::String description) noexcept :
        _title{std::move(title)}, _description{std::move(description)} {}

public: // accessors
    /// Get the short error title.
    /// @return The title.
    [[nodiscard]] auto title() const noexcept -> const text::String & { return _title; }
    /// Get the user-facing error description.
    /// @return The description.
    [[nodiscard]] auto description() const noexcept -> const text::String & { return _description; }
    /// Get the optional local endpoint.
    /// @return The local endpoint, or `std::nullopt` if it is unavailable.
    [[nodiscard]] auto localEndpoint() const noexcept -> const std::optional<IpEndpoint> & { return _localEndpoint; }
    /// Get the optional remote endpoint.
    /// @return The remote endpoint, or `std::nullopt` if it is unavailable.
    [[nodiscard]] auto remoteEndpoint() const noexcept -> const std::optional<HostEndpoint> & {
        return _remoteEndpoint;
    }
    /// Get the optional platform error context.
    /// @return The platform context, or a null pointer if it is unavailable.
    [[nodiscard]] auto platformContext() const noexcept -> const system::PlatformErrorContextConstPtr & {
        return _platformContext;
    }
    /// Set the local endpoint.
    /// @param endpoint The local endpoint involved in the operation.
    /// @return This context for chaining.
    auto setLocalEndpoint(IpEndpoint endpoint) noexcept -> NetworkErrorContext & {
        _localEndpoint = std::move(endpoint);
        return *this;
    }
    /// Set the remote endpoint.
    /// @param endpoint The remote endpoint involved in the operation.
    /// @return This context for chaining.
    auto setRemoteEndpoint(HostEndpoint endpoint) noexcept -> NetworkErrorContext & {
        _remoteEndpoint = std::move(endpoint);
        return *this;
    }
    /// Set the platform error context.
    /// @param context The platform-specific error details.
    /// @return This context for chaining.
    auto setPlatformContext(system::PlatformErrorContextConstPtr context) noexcept -> NetworkErrorContext & {
        _platformContext = std::move(context);
        return *this;
    }

private:
    text::String _title;                                   ///< The short error title.
    text::String _description;                             ///< The user-facing description.
    std::optional<IpEndpoint> _localEndpoint;              ///< The optional local endpoint.
    std::optional<HostEndpoint> _remoteEndpoint;           ///< The optional remote endpoint.
    system::PlatformErrorContextConstPtr _platformContext; ///< The optional platform details.
};

}
