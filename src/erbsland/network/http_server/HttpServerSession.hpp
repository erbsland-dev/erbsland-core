// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServerSession_fwd.hpp"
#include "HttpServerSessionEventEditor.hpp"
#include "HttpSessionData.hpp"

#include "../../event/EventSource.hpp"
#include "../../text/String.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// A logical HTTP server session that can span multiple connections.
/// @tested{HttpServerSessionTest HttpServerLiveTest}
class HttpServerSession : public event::EventSource {
public:
    // defaults
    ~HttpServerSession() override = default;

public:
    /// Get the optional manager-assigned identifier.
    [[nodiscard]] virtual auto identifier() const -> std::optional<text::String> = 0;
    /// Get the application-defined session data pointer.
    [[nodiscard]] virtual auto data() const noexcept -> const HttpSessionDataPtr & = 0;
    /// Replace application-defined session data on the owner loop.
    virtual void setData(HttpSessionDataPtr data) = 0;
    /// Test whether this session can be selected for new requests.
    [[nodiscard]] virtual auto isValid() const noexcept -> bool = 0;
    /// Prevent new selection and notify the session manager.
    virtual void invalidate() = 0;
    /// Access session-specific routes and lifecycle callbacks.
    [[nodiscard]] auto events() -> HttpServerSessionEventEditor & override = 0;

protected:
    /// Create an abstract logical session owned by one event target.
    explicit HttpServerSession(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
