// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Host.hpp"
#include "Network_fwd.hpp"
#include "NetworkSourceState.hpp"

#include "../event/EventSource.hpp"

namespace erbsland::network {

/// An inactive, one-shot asynchronous host lookup.
/// @notest{Abstract interface; mock and native lookup implementations own behavior tests.}
class HostLookup : public event::EventSource {
public:
    // defaults
    ~HostLookup() override = default;

public:
    /// Get the host being resolved.
    /// @return The configured host.
    [[nodiscard]] virtual auto host() const noexcept -> const Host & = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start the inactive lookup.
    virtual void start() = 0;
    /// Cancel the lookup immediately.
    virtual void cancel() noexcept = 0;
    /// Create a retained callback editor.
    /// @return A new callback editor connected to this lookup.
    [[nodiscard]] virtual auto events() -> HostLookupEventsPtr = 0;

protected:
    /// Create a lookup owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit HostLookup(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
