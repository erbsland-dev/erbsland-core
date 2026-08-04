// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TcpConnectionRequestFn.hpp"
#include "TcpListenerEventEditor_fwd.hpp"

#include "../source/NetworkErrorFn.hpp"
#include "../source/NetworkEventFn.hpp"

#include "../../event/impl/CommonEventEditor.hpp"

#include <utility>

namespace erbsland::network {

/// Callback editor for a TCP listener.
/// The listener owns this stable editor; its common base exposes the source and callback target to generic code.
/// @notest{Abstract interface; mock and native listener implementations own behavior tests.}
class TcpListenerEventEditor : public event::impl::CommonEventEditor {
public:
    // defaults
    ~TcpListenerEventEditor() override = default;

public:
    /// Set the listening-ready callback.
    /// @param callback The callback invoked after binding and listening succeeds.
    /// @return This editor for chaining.
    virtual auto onListening(NetworkEventFn callback) -> TcpListenerEventEditor & = 0;
    /// Set the incoming-connection callback.
    /// @param callback The callback receiving pending requests.
    /// @return This editor for chaining.
    virtual auto onConnection(TcpConnectionRequestFn callback) -> TcpListenerEventEditor & = 0;
    /// Set the closure callback.
    /// @param callback The callback invoked after the listener closes.
    /// @return This editor for chaining.
    virtual auto onClosed(NetworkEventFn callback) -> TcpListenerEventEditor & = 0;
    /// Set the operational-error callback.
    /// @param callback The callback receiving the error.
    /// @return This editor for chaining.
    virtual auto onError(NetworkErrorFn callback) -> TcpListenerEventEditor & = 0;
    /// Set the final callback.
    /// @param callback The callback invoked after closure, failure, or explicit abort.
    /// @return This editor for chaining.
    virtual auto onFinal(NetworkEventFn callback) -> TcpListenerEventEditor & = 0;

protected:
    /// Create an editor for a TCP listener.
    /// @param source The listener that owns the editor.
    /// @param target The callback target.
    TcpListenerEventEditor(event::EventSourcePtr source, event::EventsPtr target) :
        CommonEventEditor{std::move(source), std::move(target)} {}
};

}
