// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientConnectionManager.hpp"
#include "HttpClientRequest_fwd.hpp"
#include "HttpClientResponseHandler.hpp"
#include "HttpClientSession_fwd.hpp"
#include "HttpClientSessionEventEditor_fwd.hpp"

#include "../cookie/HttpCookieJar.hpp"

#include "../../../http_client/HttpClientSession.hpp"

#include <cstdint>
#include <deque>
#include <memory>
#include <vector>

namespace erbsland::network::impl {

/// Built-in session-first HTTP/HTTPS client.
/// @tested{HttpClientTest}
class HttpClientSession final : public network::HttpClientSession {
    friend class HttpClientRequest;
    friend class HttpClientSessionEventEditor;

public:
    /// Create an active client session owned by one event loop.
    explicit HttpClientSession(event::EventsPtr ownerEvents);
    ~HttpClientSession() override;

public: // implement network::HttpClientSession
    void setOptions(HttpClientSessionOptions options) override;
    void setTlsOptions(HttpClientTlsOptions options) override;
    void setRedirectOptions(HttpClientRedirectOptions options) override;
    void setDefaultHeaders(HttpHeaders headers) override;
    [[nodiscard]] auto cookieJar() noexcept -> network::HttpCookieJar & override;
    [[nodiscard]] auto createRequest(Url url) -> HttpClientRequestPtr override;
    [[nodiscard]] auto createRequest(HttpMethod method, Url url) -> HttpClientRequestPtr override;
    void sendRequest(HttpClientRequestPtr request) override;
    [[nodiscard]] auto sendGet(Url url) -> HttpClientRequestPtr override;
    [[nodiscard]] auto sendHead(Url url) -> HttpClientRequestPtr override;
    [[nodiscard]] auto sendPost(Url url, mem::ByteBlock body, HttpHeaders headers) -> HttpClientRequestPtr override;
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    void close() override;
    void abort() noexcept override;
    [[nodiscard]] auto events() -> network::HttpClientSessionEventEditor & override;

private:
    /// Validate session configuration before capture.
    void validateConfiguration() const;
    /// Start as many queued requests as capacity permits.
    void dispatch();
    /// Remove a finalized active request and continue dispatch.
    void requestFinalized(const std::shared_ptr<HttpClientRequest> &request);
    /// Release a request's retained fixed-body quota.
    void releaseBodyQuota(unit::ByteLength length) noexcept;
    /// Replace one active request's retained fixed-body reservation.
    [[nodiscard]] auto replaceBodyQuota(unit::ByteLength oldLength, unit::ByteLength newLength) noexcept -> bool;
    /// Finish graceful session closure after all submitted requests finalize.
    void assessClosed();
    /// Emit session finalization exactly once.
    void finishFinal();

private:
    std::unique_ptr<HttpClientSessionEventEditor> _eventEditor; ///< Stable public editor.
    std::deque<std::shared_ptr<HttpClientRequest>> _pending;    ///< FIFO submitted requests.
    std::vector<std::shared_ptr<HttpClientRequest>> _active;    ///< Active dedicated connections.
    HttpClientSessionOptions _options;                          ///< Defaults captured per submission.
    HttpClientTlsOptions _tlsOptions;                           ///< HTTPS defaults captured per submission.
    HttpClientRedirectOptions _redirectOptions;                 ///< Redirect defaults captured per submission.
    HttpHeaders _defaultHeaders;                                ///< Default request fields.
    HttpCookieJar _cookieJar;                                   ///< Stable cookie storage.
    HttpClientConnectionManagerPtr _connectionManager;          ///< Sequential connection reuse.
    HttpClientResponseHandler _responseHandler;                 ///< Default response handling.
    HttpClientInformationalResponseFn _onInformational;         ///< Informational-response fallback.
    HttpClientRedirectFn _onRedirect;                           ///< Redirect-policy fallback.
    HttpClientErrorFn _onError;                                 ///< Request-error fallback.
    NetworkEventFn _onClosed;                                   ///< Graceful closure callback.
    NetworkEventFn _onFinal;                                    ///< Exactly-once final callback.
    NetworkSourceState _state{NetworkSourceState::Active};      ///< Session lifecycle.
    std::uint64_t _retainedBodyLength{};                        ///< Submitted fixed-body bytes.
    std::uint64_t _configurationGeneration{1U};                 ///< Transport/reuse configuration generation.
    bool _finalEmitted{};                                       ///< Finalization guard.
};

}
