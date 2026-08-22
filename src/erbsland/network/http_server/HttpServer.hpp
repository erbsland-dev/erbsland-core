// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpServer_fwd.hpp"
#include "HttpServerEventEditor.hpp"
#include "HttpServerOptions.hpp"
#include "HttpServerSessionManager_fwd.hpp"
#include "HttpServerTlsOptions.hpp"
#include "HttpStaticContentHandler_fwd.hpp"

#include "../IpEndpoint.hpp"
#include "../source/NetworkSourceState.hpp"
#include "../tcp/TcpAcceptOptions.hpp"
#include "../tcp/TcpListenerOptions.hpp"

#include "../../event/EventSource.hpp"

#include <optional>
#include <utility>

namespace erbsland::network {

/// A configurable session-first HTTP/1.1 or HTTPS server.
/// @tested{HttpServerLiveTest NetworkFacadeTest}
class HttpServer : public event::EventSource {
public:
    // defaults
    ~HttpServer() override = default;

public: // configuration
    /// Replace HTTP limits and deadlines while inactive.
    virtual void setOptions(HttpServerOptions options) = 0;
    /// Replace listener admission and socket options while inactive.
    virtual void setListenerOptions(TcpListenerOptions options) = 0;
    /// Replace accepted TCP stream options for plaintext and HTTPS while inactive.
    virtual void setTcpAcceptOptions(TcpAcceptOptions options) = 0;
    /// Enable HTTPS with secure HTTP defaults while inactive.
    virtual void enableTls() = 0;
    /// Enable HTTPS with curated server options while inactive.
    virtual void enableTls(HttpServerTlsOptions options) = 0;
    /// Replace the synchronous logical-session manager while inactive.
    virtual void setSessionManager(HttpServerSessionManagerPtr manager) = 0;
    /// Add one server-level static-content handler while inactive.
    virtual void addStaticContentHandler(HttpStaticContentHandlerPtr handler) = 0;

public: // lifecycle
    /// Get the effective local endpoint after the listener binds.
    [[nodiscard]] virtual auto localEndpoint() const -> std::optional<IpEndpoint> = 0;
    /// Get the server lifecycle state.
    [[nodiscard]] virtual auto state() const noexcept -> NetworkSourceState = 0;
    /// Bind and start after capturing configuration and routes.
    virtual void start(IpEndpoint localEndpoint) = 0;
    /// Temporarily stop native acceptance without closing active sessions.
    virtual void pauseAccepting() = 0;
    /// Resume native acceptance while active.
    virtual void resumeAccepting() = 0;
    /// Stop accepting, disable reuse, and drain committed responses.
    virtual void close() = 0;
    /// Terminate listener and connections immediately.
    virtual void abort() noexcept = 0;
    /// Access the stable callback and route editor on the owner loop.
    [[nodiscard]] auto events() -> HttpServerEventEditor & override = 0;

protected:
    /// Create an abstract server source owned by one event target.
    explicit HttpServer(event::EventsPtr ownerEvents) : EventSource{std::move(ownerEvents)} {}
};

}
