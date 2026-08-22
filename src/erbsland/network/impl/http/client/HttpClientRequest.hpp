// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "HttpClientRequest_fwd.hpp"
#include "HttpClientRequestEventEditor_fwd.hpp"
#include "HttpClientResponse_fwd.hpp"
#include "HttpClientResponseHandler.hpp"
#include "HttpClientSession_fwd.hpp"

#include "../codec/Http1DecodeEvent_fwd.hpp"
#include "../codec/Http1Transaction_fwd.hpp"
#include "../codec/Http1TransactionFailure_fwd.hpp"

#include "../../../../unit/ByteIndex.hpp"
#include "../../../http_client/HttpClientRequest.hpp"
#include "../../../http_client/HttpClientSessionOptions.hpp"
#include "../../../http_client/HttpClientTlsOptions.hpp"
#include "../../../source/Connection_fwd.hpp"
#include "../../../source/NetworkErrorContext_fwd.hpp"
#include "../../../source/NetworkErrorPhase.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace erbsland::network::impl {

/// Built-in prepared and submitted HTTP client request.
/// @tested{HttpClientTest}
class HttpClientRequest final : public network::HttpClientRequest {
    friend class HttpClientRequestEventEditor;
    friend class HttpClientResponse;
    friend class HttpClientSession;

private:
    /// Outgoing request-body mode.
    enum class BodyMode : std::uint8_t {
        Empty,    ///< No request body.
        Fixed,    ///< One retained fixed body.
        Streaming ///< Caller-produced chunked body.
    };

public:
    /// Create one prepared request associated with a session.
    HttpClientRequest(std::shared_ptr<HttpClientSession> session, HttpMethod method, Url url);
    ~HttpClientRequest() override;

public: // implement network::HttpClientRequest
    [[nodiscard]] auto method() const noexcept -> const HttpMethod & override;
    [[nodiscard]] auto url() const noexcept -> const Url & override;
    [[nodiscard]] auto headers() const noexcept -> const HttpHeaders & override;
    void setHeaders(HttpHeaders headers) override;
    void setBody(mem::ByteBlock body) override;
    void streamBody() override;
    void setRedirectOptions(HttpClientRedirectOptions options) override;
    [[nodiscard]] auto sendBody(const mem::ByteBlock &data) -> NetworkSendStatus override;
    [[nodiscard]] auto finishBody(HttpHeaders trailers) -> NetworkSendStatus override;
    [[nodiscard]] auto state() const noexcept -> NetworkSourceState override;
    void cancel() noexcept override;
    [[nodiscard]] auto events() -> network::HttpClientRequestEventEditor & override;

private: // session orchestration
    /// Capture session configuration and enter the pending state.
    void submit(
        HttpClientSessionOptions options,
        HttpClientTlsOptions tlsOptions,
        HttpClientRedirectOptions redirectOptions,
        HttpHeaders defaultHeaders,
        HttpClientResponseHandler responseHandler,
        HttpClientInformationalResponseFn informational,
        HttpClientRedirectFn redirect,
        HttpClientErrorFn error,
        std::uint64_t configurationGeneration);
    /// Start the dedicated TCP or TLS connection.
    void start();
    /// Fail a request rejected before it becomes active.
    void failQueued(NetworkErrorContext context);

private: // transport and transaction
    /// Handle resolution completion and start the independent TCP phase timer.
    void handleHostResolved();
    /// Start the HTTP transaction after plaintext or TLS activation.
    void handleConnectionActive();
    /// End the independent TCP phase when the TLS transport is established.
    void handleTransportConnected();
    /// Handle setup failure before the transaction owns connection callbacks.
    void handleConnectionError(const NetworkErrorContext &context);
    /// Build and start the internal transaction.
    void startTransaction();
    /// Pump the retained fixed body and finish empty/fixed request framing.
    void pumpPreparedBody();
    /// Handle a final response-head checkpoint.
    void handleResponseHead(const Http1DecodeEvent &event);
    /// Handle one informational response.
    void handleInformational(const Http1DecodeEvent &event);
    /// Handle internal transaction failure.
    void handleTransactionFailure(const Http1TransactionFailure &failure);
    /// Handle transaction finalization and close a reusable dedicated connection.
    void handleTransactionFinal();
    /// Continue request finalization after asynchronous response processing.
    void handleResponseFinal();
    /// Complete transport reuse/closure after both transaction and response finalize.
    void completeTransactionFinal();
    /// Evaluate one eligible redirect response and select its action.
    [[nodiscard]] auto handleRedirect(const Http1DecodeEvent &event, bool hasBody) -> bool;
    /// Start the prepared next redirect hop after the intermediate body is drained.
    void startRedirectHop();
    /// Build a canonical fragment-free URL used for redirect loop detection.
    [[nodiscard]] static auto canonicalRequestUrl(const Url &url) -> text::String;
    /// Test whether two URLs have the same HTTP origin.
    [[nodiscard]] static auto sameOrigin(const Url &first, const Url &second) noexcept -> bool;
    /// Permanently remove fields that cannot cross an origin boundary.
    static void stripOriginBoundFields(HttpHeaders &headers);
    /// Remove representation metadata after rewriting the request to an empty body.
    static void stripBodyFields(HttpHeaders &headers);
    /// Test whether the current request may be retried after a stale idle lease.
    [[nodiscard]] auto canRetryStale() const noexcept -> bool;

private: // lifecycle and failures
    /// Handle a generation-checked overall timeout.
    void handleOverallTimeout(std::uint64_t generation);
    /// Handle a generation-checked phase timeout.
    void handlePhaseTimeout(std::uint64_t generation, NetworkErrorPhase phase);
    /// Emit an operational request failure exactly once.
    void fail(NetworkErrorContext context);
    /// Finish request and response finalization exactly once.
    void finishFinal();
    /// Require the prepared state.
    void verifyPrepared() const;
    /// Validate URL, headers, limits, and body mode at submission.
    void validateSubmission() const;
    /// Build default/request fields plus framework-controlled framing fields.
    [[nodiscard]] auto buildHeaders() const -> HttpHeaders;
    /// Build the canonical origin-form request target.
    [[nodiscard]] auto requestTarget() const -> text::String;
    /// Build the canonical Host field value.
    [[nodiscard]] auto hostFieldValue() const -> text::String;

private:
    std::weak_ptr<HttpClientSession> _session;                        ///< Creating session.
    std::unique_ptr<HttpClientRequestEventEditor> _eventEditor;       ///< Stable public editor.
    HttpMethod _method;                                               ///< Immutable request method.
    Url _url;                                                         ///< Immutable absolute URL.
    HttpMethod _hopMethod;                                            ///< Current-hop method.
    Url _effectiveUrl;                                                ///< Current-hop URL.
    Url _connectionUrl;                                               ///< Origin of the current physical connection.
    HttpHeaders _headers;                                             ///< Request-specific fields.
    HttpHeaders _hopHeaders;                                          ///< Current-hop request fields.
    mem::ByteBlock _fixedBody;                                        ///< Optional retained fixed body.
    BodyMode _bodyMode{BodyMode::Empty};                              ///< Prepared outgoing body mode.
    HttpClientSessionOptions _options;                                ///< Captured session options.
    HttpClientTlsOptions _tlsOptions;                                 ///< Captured HTTPS options.
    HttpClientRedirectOptions _redirectOptions;                       ///< Captured redirect policy.
    std::optional<HttpClientRedirectOptions> _requestRedirectOptions; ///< Prepared redirect override.
    HttpHeaders _defaultHeaders;                                      ///< Captured default fields.
    HttpClientResponseHandler _responseHandler;                       ///< Captured or request response handling.
    std::optional<HttpClientResponseHandler> _requestResponseHandler; ///< Request-specific override.
    HttpClientInformationalResponseFn _onInformational;               ///< Captured fallback.
    HttpClientInformationalResponseFn _requestInformational;          ///< Request override.
    HttpClientRedirectFn _onRedirect;                                 ///< Captured fallback.
    HttpClientRedirectFn _requestRedirect;                            ///< Request override.
    HttpClientErrorFn _onError;                                       ///< Captured fallback.
    HttpClientErrorFn _requestError;                                  ///< Request override.
    NetworkEventFn _onWritable;                                       ///< Upload writable callback.
    NetworkEventFn _onFinal;                                          ///< Exactly-once final callback.
    ConnectionPtr _connection;                                        ///< Dedicated plaintext or TLS connection.
    Http1TransactionPtr _transaction;                                 ///< Internal HTTP/1 transaction.
    std::shared_ptr<HttpClientResponse> _response;                    ///< Retained final response.
    unit::ByteIndex _fixedBodyOffset;                                 ///< Next fixed-body byte.
    NetworkSourceState _state{NetworkSourceState::Inactive};          ///< Request lifecycle.
    std::uint64_t _overallGeneration{};                               ///< Invalidates the overall timer.
    std::uint64_t _phaseTimerGeneration{};                            ///< Invalidates phase timers.
    std::uint64_t _configurationGeneration{};                         ///< Captured reuse compatibility generation.
    unit::ItemCount _completedTransactions;                           ///< Transactions on the leased connection.
    unit::ItemCount _redirectCount;                                   ///< Followed redirect count.
    unit::ByteLength _retainedBodyLength;                             ///< Session quota reserved by this request.
    std::vector<text::String> _redirectHistory;                       ///< Canonical loop-detection URLs.
    bool _submitted{};                                                ///< Submission guard.
    bool _uploadWritablePending{};                                    ///< Initial or renewed writable owed.
    bool _uploadFinished{};                                           ///< Request body finish accepted.
    bool _cancelled{};                                                ///< Caller cancellation suppresses errors.
    bool _failed{};                                                   ///< Error guard.
    bool _finalEmitted{};                                             ///< Finalization guard.
    bool _followingRedirect{};                                        ///< Intermediate body is being drained.
    bool _leasedReused{};         ///< Current connection came from the idle manager.
    bool _responseObserved{};     ///< Any informational/final head was observed.
    bool _retryStale{};           ///< Retry after current stale lease finalizes.
    bool _staleRetried{};         ///< One stale retry was already consumed.
    bool _transactionFinalized{}; ///< HTTP engine reached its final callback.
};

}
