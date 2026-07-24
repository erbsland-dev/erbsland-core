// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Network_fwd.hpp"
#include "NetworkCallbacks.hpp"

#include "../event/EventEditor.hpp"

namespace erbsland::network {

/// Callback editor for a host lookup.
/// @notest{Abstract interface; mock and native lookup implementations own behavior tests.}
class HostLookupEvents : public event::EventEditor {
public:
    // defaults
    ~HostLookupEvents() override = default;

public:
    /// Set the successful-resolution callback.
    /// @param callback The callback receiving the resolved addresses.
    /// @return This editor for chaining.
    virtual auto onResolved(HostResolvedFn callback) -> HostLookupEvents & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> HostLookupEvents & = 0;

protected:
    /// Create an editor for an event collection.
    /// @param targetEvents The target event collection.
    explicit HostLookupEvents(event::EventsPtr targetEvents) : EventEditor{std::move(targetEvents)} {}
};

}
