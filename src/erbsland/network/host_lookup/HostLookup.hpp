// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookup_fwd.hpp"
#include "HostLookupEventEditor.hpp"
#include "HostLookupOptions.hpp"

#include "../Host.hpp"
#include "../source/NetworkSourceState.hpp"

#include "../../event/EventSource.hpp"

#include <optional>

namespace erbsland::network {

/// A reusable asynchronous host lookup.
/// @notest{Abstract interface; mock and native lookup implementations own behavior tests.}
class HostLookup : public event::EventSource {
public:
    // defaults
    ~HostLookup() override = default;

public:
    /// Get the host of the current operation.
    /// This method is thread-safe.
    /// @return The active host, or `std::nullopt` if no operation is active.
    [[nodiscard]] virtual auto host() const -> std::optional<Host> = 0;
    /// Get the source lifecycle state.
    /// @return The current state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Start an inactive lookup operation.
    /// Completion is always delivered asynchronously on the owner event loop.
    /// @param host The address or unresolved name to resolve.
    /// @param options The options for this operation.
    /// @throws err::LogicError If called outside the owner event loop or while an operation is active.
    /// @throws err::ParameterError If the timeout or attempt count is not positive, or the retry delay is negative.
    virtual void start(Host host, HostLookupOptions options = {}) = 0;
    /// Cancel the lookup immediately.
    /// This method is thread-safe and suppresses completion that has not started dispatching.
    virtual void cancel() noexcept = 0;
    /// Access the editor for the lookup-owned event handlers.
    /// @return The stable source-owned editor.
    /// @throws err::LogicError If called outside the owner event loop.
    [[nodiscard]] auto events() -> HostLookupEventEditor & override = 0;

protected:
    /// Create a lookup owned by an event collection.
    /// @param ownerEvents The owner-loop event collection.
    explicit HostLookup(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
