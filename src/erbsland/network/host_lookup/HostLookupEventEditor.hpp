// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HostLookupEventEditor_fwd.hpp"
#include "HostResolvedFn.hpp"

#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Editor for the event handlers owned by a host lookup.
/// The lookup owns this editor and its handlers. The common editor base exposes the lookup and callback target to
/// generic event-editor code without introducing a strong ownership cycle.
/// @tested{HostLookupTest NetworkFacadeTest}
class HostLookupEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~HostLookupEventEditor() override = default;

public:
    /// Set the successful-resolution handler.
    /// @param callback The callback receiving the resolved addresses, or an empty callback to clear the handler.
    /// @return This editor for chaining.
    virtual auto onResolved(HostResolvedFn callback) -> HostLookupEventEditor & = 0;
    /// Set the operational-error handler.
    /// @param callback The callback receiving the error, or an empty callback to clear the handler.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> HostLookupEventEditor & = 0;
    /// Set the final handler.
    /// This handler runs after a successful, failed, or cancelled operation returns the lookup to its inactive state.
    /// @param callback The final callback, or an empty callback to clear the handler.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> HostLookupEventEditor & = 0;

protected:
    /// Create an editor for a host lookup.
    /// @param source The lookup that owns the editor.
    /// @param target The callback target.
    HostLookupEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
