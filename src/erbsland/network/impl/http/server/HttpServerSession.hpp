// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpRoutes.hpp"
#include "HttpServerSession_fwd.hpp"
#include "HttpServerSessionEventEditor_fwd.hpp"

#include "../../../http/HttpHeaders.hpp"
#include "../../../http_server/HttpServerSession.hpp"

#include <functional>
#include <memory>
#include <optional>

namespace erbsland::network::impl {

/// Built-in logical HTTP server session.
/// @tested{HttpServerSessionTest HttpServerLiveTest}
class HttpServerSession final : public network::HttpServerSession {
    friend class HttpServerSessionEventEditor;

public:
    /// Callback invoked after the session becomes invalid.
    using InvalidatedFn = std::function<void(const network::HttpServerSessionPtr &)>;
    /// Callback invoked to renew the manager-owned identifier.
    using RenewedFn = std::function<bool(const network::HttpServerSessionPtr &)>;

public:
    /// Create one server-owned session with optional identity and data.
    HttpServerSession(
        event::EventsPtr ownerEvents,
        std::optional<text::String> identifier,
        HttpSessionDataPtr data,
        InvalidatedFn invalidatedFn,
        RenewedFn renewedFn);
    ~HttpServerSession() override;

public: // implement network::HttpServerSession
    /// Get the optional stable identity.
    [[nodiscard]] auto identifier() const -> std::optional<text::String> override;
    /// Get application-defined session data.
    [[nodiscard]] auto data() const noexcept -> const HttpSessionDataPtr & override;
    /// Replace application-defined session data.
    void setData(HttpSessionDataPtr data) override;
    /// Test whether new requests may select this session.
    [[nodiscard]] auto isValid() const noexcept -> bool override;
    /// Invalidate once and emit terminal session callbacks.
    void invalidate() override;
    [[nodiscard]] auto renewIdentifier() -> bool override;
    /// Access the stable public session editor.
    [[nodiscard]] auto events() -> network::HttpServerSessionEventEditor & override;

public: // internal
    /// Access mutable session-priority routes.
    [[nodiscard]] auto routes() noexcept -> HttpRoutes & { return _routes; }
    /// Access session-priority routes.
    [[nodiscard]] auto routes() const noexcept -> const HttpRoutes & { return _routes; }
    /// Capture manager fields emitted after invalidation.
    void setInvalidationResponseFields(HttpHeaders fields) { _invalidationResponseFields = std::move(fields); }
    /// Get manager fields emitted after invalidation.
    [[nodiscard]] auto invalidationResponseFields() const noexcept -> const HttpHeaders & {
        return _invalidationResponseFields;
    }
    /// Replace the manager identifier and retain fields for the next response.
    void setRenewal(std::optional<text::String> identifier, HttpHeaders fields);
    /// Consume manager fields produced by identifier renewal.
    [[nodiscard]] auto takeRenewalResponseFields() -> HttpHeaders;
    /// Deliver a request selected for this session.
    void deliverRequest(network::HttpServerRequestPtr request);

private:
    std::optional<text::String> _identifier;                    ///< Optional stable manager identity.
    HttpSessionDataPtr _data;                                   ///< Application-defined state.
    InvalidatedFn _invalidatedFn;                               ///< Server/manager invalidation observer.
    RenewedFn _renewedFn;                                       ///< Server/manager renewal adapter.
    std::unique_ptr<HttpServerSessionEventEditor> _eventEditor; ///< Stable editor.
    HttpRoutes _routes;                                         ///< Session-priority routes.
    HttpHeaders _invalidationResponseFields;                    ///< Fields appended after invalidation.
    HttpHeaders _renewalResponseFields;                         ///< Fields appended once after renewal.
    HttpServerRequestEventFn _onRequestReceived;                ///< Selected request callback.
    NetworkEventFn _onInvalidated;                              ///< Invalidation observer.
    NetworkEventFn _onFinal;                                    ///< Final observer.
    bool _valid{true};                                          ///< Whether new requests may select this session.
    bool _finalEmitted{};                                       ///< Exactly-once final guard.
};

}
