// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientRequest.hpp"

#include "HttpClientRequestEventEditor.hpp"
#include "HttpClientResponse.hpp"
#include "HttpClientSession.hpp"

#include "../codec/Http1CodecLimits.hpp"
#include "../codec/Http1DecodeEvent.hpp"
#include "../codec/Http1Transaction.hpp"
#include "../codec/Http1TransactionFailure.hpp"
#include "../codec/Http1TransactionOptions.hpp"
#include "../cookie/PublicSuffixList.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpRequestHead.hpp"
#include "../../../Network.hpp"
#include "../../../source/Connection.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorPhase.hpp"
#include "../../../source/NetworkErrorReason.hpp"
#include "../../../tcp/TcpConnection.hpp"
#include "../../../tcp/TcpConnectionEventEditor.hpp"
#include "../../../tcp/TcpConnectOptions.hpp"
#include "../../../tls/TlsClientConnection.hpp"
#include "../../../tls/TlsClientConnectionEventEditor.hpp"
#include "../../../tls/TlsClientConnectOptions.hpp"
#include "../../ConnectionProtocolAccess.hpp"
#include "../../url/UrlData.hpp"
#include "../../url/UrlWriter.hpp"

#include <algorithm>
#include <ranges>
#include <utility>
#include <vector>

namespace erbsland::network::impl {

using namespace text::literals;

HttpClientRequest::HttpClientRequest(std::shared_ptr<HttpClientSession> session, HttpMethod method, Url url) :
    network::HttpClientRequest{session->ownerEvents()},
    _session{std::move(session)},
    _method{std::move(method)},
    _url{std::move(url)},
    _hopMethod{_method},
    _effectiveUrl{_url} {
}

HttpClientRequest::~HttpClientRequest() {
    if (_transaction != nullptr) {
        _transaction->cancel();
    } else if (_connection != nullptr) {
        _connection->abort();
    }
}

auto HttpClientRequest::method() const noexcept -> const HttpMethod & {
    return _method;
}

auto HttpClientRequest::url() const noexcept -> const Url & {
    return _url;
}

auto HttpClientRequest::headers() const noexcept -> const HttpHeaders & {
    return _headers;
}

void HttpClientRequest::setHeaders(HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    verifyPrepared();
    for (const auto &field : headers.fields()) {
        const auto type = field.name().type();
        if (type == HttpFieldType::Host || type == HttpFieldType::ContentLength ||
            type == HttpFieldType::TransferEncoding || type == HttpFieldType::Expect ||
            type == HttpFieldType::Upgrade) {
            throw err::ParameterError{
                "Host, framing, Expect, and Upgrade fields are controlled by the HTTP client."_el, "headers"_el};
        }
    }
    _headers = std::move(headers);
}

void HttpClientRequest::setBody(mem::ByteBlock body) {
    verifyCurrentOwnerEvents();
    verifyPrepared();
    _fixedBody = std::move(body);
    _bodyMode = BodyMode::Fixed;
}

void HttpClientRequest::streamBody() {
    verifyCurrentOwnerEvents();
    verifyPrepared();
    _fixedBody = {};
    _bodyMode = BodyMode::Streaming;
}

void HttpClientRequest::setRedirectOptions(HttpClientRedirectOptions options) {
    verifyCurrentOwnerEvents();
    verifyPrepared();
    if (options.maximumRedirects().isZero() || options.maximumRedirects().isInfinite()) {
        throw err::ParameterError{"The redirect limit must be positive and finite."_el, "options"_el};
    }
    _requestRedirectOptions = options;
}

auto HttpClientRequest::sendBody(const mem::ByteBlock &data) -> NetworkSendStatus {
    verifyCurrentOwnerEvents();
    if (_bodyMode != BodyMode::Streaming || _failed || _finalEmitted || _uploadFinished) {
        return NetworkSendStatus::Closed;
    }
    if (_transaction == nullptr || _state != NetworkSourceState::Active) {
        _uploadWritablePending = true;
        return NetworkSendStatus::WouldBlock;
    }
    const auto result = _transaction->sendBody(data);
    _uploadWritablePending = result.wouldBlock();
    return result;
}

auto HttpClientRequest::finishBody(HttpHeaders trailers) -> NetworkSendStatus {
    verifyCurrentOwnerEvents();
    if (_bodyMode != BodyMode::Streaming || _failed || _finalEmitted || _uploadFinished) {
        return NetworkSendStatus::Closed;
    }
    if (_transaction == nullptr || _state != NetworkSourceState::Active) {
        _uploadWritablePending = true;
        return NetworkSendStatus::WouldBlock;
    }
    const auto result = _transaction->finishBody(std::move(trailers));
    _uploadWritablePending = result.wouldBlock();
    _uploadFinished = result.isAccepted();
    return result;
}

auto HttpClientRequest::state() const noexcept -> NetworkSourceState {
    return _state;
}

void HttpClientRequest::cancel() noexcept {
    if (_finalEmitted) {
        return;
    }
    _cancelled = true;
    _state = NetworkSourceState::Closed;
    ++_overallGeneration;
    ++_phaseTimerGeneration;
    if (_response != nullptr && !_response->isFinal()) {
        _response->finishFinal();
    }
    if (_transaction != nullptr) {
        _transaction->cancel();
        return;
    }
    if (_connection != nullptr) {
        _connection->abort();
    }
    finishFinal();
}

auto HttpClientRequest::events() -> network::HttpClientRequestEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpClientRequestEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

void HttpClientRequest::submit(
    HttpClientSessionOptions options,
    HttpClientTlsOptions tlsOptions,
    HttpClientRedirectOptions redirectOptions,
    HttpHeaders defaultHeaders,
    HttpClientResponseHandler responseHandler,
    HttpClientInformationalResponseFn informational,
    HttpClientRedirectFn redirect,
    HttpClientErrorFn error,
    const std::uint64_t configurationGeneration) {
    verifyPrepared();
    _options = std::move(options);
    _tlsOptions = std::move(tlsOptions);
    _redirectOptions = _requestRedirectOptions.value_or(redirectOptions);
    _defaultHeaders = std::move(defaultHeaders);
    _responseHandler = _requestResponseHandler.value_or(std::move(responseHandler));
    _onInformational = _requestInformational ? _requestInformational : std::move(informational);
    _onRedirect = _requestRedirect ? _requestRedirect : std::move(redirect);
    _onError = _requestError ? _requestError : std::move(error);
    _configurationGeneration = configurationGeneration;
    validateSubmission();
    _hopMethod = _method;
    _effectiveUrl = _url;
    _hopHeaders = _headers;
    _redirectHistory = {canonicalRequestUrl(_effectiveUrl)};
    _submitted = true;
    _state = NetworkSourceState::Starting;
    const auto generation = ++_overallGeneration;
    const auto weakSelf =
        std::weak_ptr<HttpClientRequest>{std::static_pointer_cast<HttpClientRequest>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.overallTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleOverallTimeout(generation);
        }
    });
}

void HttpClientRequest::start() {
    if (_finalEmitted || _failed) {
        return;
    }
    _connectionUrl = _effectiveUrl;
    if (const auto session = _session.lock(); session && _options.connectionReuseEnabled()) {
        if (auto lease = session->_connectionManager->lease(_effectiveUrl, _configurationGeneration)) {
            _connection = std::move(lease->connection);
            _completedTransactions = lease->completedTransactions;
            _leasedReused = true;
            _state = NetworkSourceState::Active;
            startTransaction();
            return;
        }
    }
    _leasedReused = false;
    _completedTransactions = {};
    const auto generation = ++_phaseTimerGeneration;
    const auto weakSelf =
        std::weak_ptr<HttpClientRequest>{std::static_pointer_cast<HttpClientRequest>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.dnsTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handlePhaseTimeout(generation, NetworkErrorPhase::Resolving);
        }
    });
    try {
        auto &network = ownerEvents()->get<Network>();
        auto tcpOptions = TcpConnectOptions{};
        auto lookupOptions = tcpOptions.hostLookupOptions();
        lookupOptions.setTimeout(_options.dnsTimeout());
        tcpOptions.setHostLookupOptions(lookupOptions)
            .setTimeout(_options.dnsTimeout() + _options.connectTimeout())
            .setBufferLimits(_options.socketBufferLimits());
        if (_effectiveUrl.scheme() == UrlScheme::Https) {
            auto connection = network.createTlsClientConnection();
            connection->events()
                .onHostResolved([weakSelf](const util::List<IpEndpoint> &) -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleHostResolved();
                    }
                })
                .onTransportConnected([weakSelf]() -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleTransportConnected();
                    }
                })
                .onHandshakeCompleted([weakSelf]() -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleConnectionActive();
                    }
                })
                .onError([weakSelf](const NetworkErrorContext &context) -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleConnectionError(context);
                    }
                });
            auto tlsOptions = TlsClientConnectOptions{};
            tlsOptions.setConfigurationLabel(_tlsOptions.configurationLabel())
                .setTcpOptions(tcpOptions)
                .setBufferLimits(_tlsOptions.bufferLimits())
                .setAlpnProtocols({"http/1.1"_el})
                .setHandshakeTimeout(_tlsOptions.handshakeTimeout())
                .setIdleTimeout(_tlsOptions.idleTimeout())
                .setCloseTimeout(_options.closeTimeout());
            _connection = connection;
            connection->connect(_effectiveUrl.endpoint(), std::move(tlsOptions));
        } else {
            auto connection = network.createTcpConnection();
            connection->events()
                .onHostResolved([weakSelf](const util::List<IpEndpoint> &) -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleHostResolved();
                    }
                })
                .onConnected([weakSelf]() -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleConnectionActive();
                    }
                })
                .onError([weakSelf](const NetworkErrorContext &context) -> void {
                    if (const auto self = weakSelf.lock()) {
                        self->handleConnectionError(context);
                    }
                });
            _connection = connection;
            connection->connect(_effectiveUrl.endpoint(), std::move(tcpOptions));
        }
    } catch (const err::Exception &error) {
        auto context = NetworkErrorContext{"HTTP client setup failed"_el, error.reason()};
        context.setReason(NetworkErrorReason::ConfigurationFailed).setPhase(NetworkErrorPhase::Configuration);
        fail(std::move(context));
        finishFinal();
    }
}

void HttpClientRequest::failQueued(NetworkErrorContext context) {
    if (_finalEmitted) {
        return;
    }
    fail(std::move(context));
    finishFinal();
}

void HttpClientRequest::handleHostResolved() {
    if (_failed || _finalEmitted) {
        return;
    }
    const auto generation = ++_phaseTimerGeneration;
    const auto weakSelf =
        std::weak_ptr<HttpClientRequest>{std::static_pointer_cast<HttpClientRequest>(shared_from_this())};
    ownerEvents()->invokeAfter(_options.connectTimeout(), [weakSelf, generation]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handlePhaseTimeout(generation, NetworkErrorPhase::Connecting);
        }
    });
}

void HttpClientRequest::handleConnectionActive() {
    if (_failed || _finalEmitted) {
        return;
    }
    ++_phaseTimerGeneration;
    _state = NetworkSourceState::Active;
    startTransaction();
}

void HttpClientRequest::handleTransportConnected() {
    if (_failed || _finalEmitted) {
        return;
    }
    ++_phaseTimerGeneration;
}

void HttpClientRequest::handleConnectionError(const NetworkErrorContext &context) {
    if (_transaction == nullptr && !_failed && !_finalEmitted) {
        fail(context);
        finishFinal();
    }
}

void HttpClientRequest::startTransaction() {
    _transactionFinalized = false;
    auto codecLimits = Http1CodecLimits{};
    codecLimits.setMaximumStartLineLength(_options.maximumStartLineLength())
        .setHeaderLimits(_options.headerLimits())
        .setTrailerLimits(_options.headerLimits())
        .setMaximumBodyLength(_options.maximumBodyLength())
        .setMaximumInputLength(_options.maximumQueueLength())
        .setMaximumOutputLength(_options.maximumQueueLength());
    auto transactionOptions = Http1TransactionOptions{};
    transactionOptions.setCodecLimits(codecLimits)
        .setHeaderTimeout(_options.headerTimeout())
        .setBodyIdleTimeout(_options.bodyIdleTimeout())
        .setTotalTimeout(_options.overallTimeout())
        .setCloseTimeout(_options.closeTimeout());
    _transaction = Http1Transaction::createClient(
        _connection,
        HttpRequestHead{_hopMethod, requestTarget(), HttpVersion::Http11, buildHeaders()},
        transactionOptions);
    const auto weakSelf =
        std::weak_ptr<HttpClientRequest>{std::static_pointer_cast<HttpClientRequest>(shared_from_this())};
    _transaction->callbacks().responseHead = [weakSelf](const Http1DecodeEvent &event) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleResponseHead(event);
        }
    };
    _transaction->callbacks().informationalHead = [weakSelf](const Http1DecodeEvent &event) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleInformational(event);
        }
    };
    _transaction->callbacks().bodyData = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleBodyData(std::move(data));
        }
    };
    _transaction->callbacks().aggregatedBody = [weakSelf](mem::ByteBlock data) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleAggregatedBody(std::move(data));
        }
    };
    _transaction->callbacks().trailers = [weakSelf](const HttpHeaders &trailers) -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleTrailers(trailers);
        }
    };
    _transaction->callbacks().inputComplete = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock(); self && self->_response) {
            self->_response->handleBodyCompleted();
        }
    };
    _transaction->callbacks().writable = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            if (self->_bodyMode == BodyMode::Fixed && !self->_uploadFinished) {
                self->pumpPreparedBody();
            } else {
                self->_uploadWritablePending = false;
                if (self->_onWritable) {
                    self->_onWritable();
                }
            }
        }
    };
    _transaction->callbacks().failure = [weakSelf](const Http1TransactionFailure &failure) -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleTransactionFailure(failure);
        }
    };
    _transaction->callbacks().final = [weakSelf]() -> void {
        if (const auto self = weakSelf.lock()) {
            self->handleTransactionFinal();
        }
    };
    _transaction->start();
    pumpPreparedBody();
}

void HttpClientRequest::pumpPreparedBody() {
    if (_transaction == nullptr || _uploadFinished) {
        return;
    }
    if (_bodyMode == BodyMode::Streaming) {
        _uploadWritablePending = false;
        if (_onWritable) {
            _onWritable();
        }
        return;
    }
    constexpr auto cChunkLength = unit::ByteLength{16U * 1024U};
    while (_bodyMode == BodyMode::Fixed && _fixedBodyOffset.isWithin(_fixedBody.length())) {
        const auto remaining = _fixedBody.length() - unit::ByteLength{_fixedBodyOffset.toRawValue()};
        const auto length = std::min(remaining, cChunkLength);
        const auto block = _fixedBody.slice(_fixedBodyOffset, length);
        const auto result = _transaction->sendBody(block);
        if (result.wouldBlock()) {
            return;
        }
        if (result.isClosed()) {
            return;
        }
        _fixedBodyOffset += length;
    }
    const auto result = _transaction->finishBody();
    _uploadFinished = result.isAccepted();
}

void HttpClientRequest::handleResponseHead(const Http1DecodeEvent &event) {
    _responseObserved = true;
    const auto hasBody = event.framing() != Http1BodyFraming::None && event.framing() != Http1BodyFraming::Opaque &&
        (!event.contentLength() || !event.contentLength()->isZero());
    if (const auto session = _session.lock(); session && _options.automaticCookiesEnabled()) {
        session->_cookieJar.storeResponseCookies(_effectiveUrl, event.response().headers());
    }
    if (handleRedirect(event, hasBody)) {
        return;
    }
    _response = std::make_shared<HttpClientResponse>(
        std::static_pointer_cast<HttpClientRequest>(shared_from_this()),
        event.response(),
        _transaction,
        hasBody,
        _effectiveUrl,
        _redirectCount,
        event.contentLength());
    if (_responseHandler.kind() == HttpClientResponseHandler::Kind::Head) {
        if (_responseHandler.hasCallback()) {
            std::get<HttpClientResponseHeadFn>(_responseHandler.callback())(
                std::static_pointer_cast<HttpClientRequest>(shared_from_this()), _response);
        } else if (hasBody) {
            _response->rejectBody();
        }
        return;
    }
    _response->startAutomatic(_responseHandler);
}

void HttpClientRequest::handleInformational(const Http1DecodeEvent &event) {
    _responseObserved = true;
    if (_onInformational) {
        _onInformational(std::static_pointer_cast<HttpClientRequest>(shared_from_this()), event.response());
    }
}

void HttpClientRequest::handleTransactionFailure(const Http1TransactionFailure &failure) {
    if (_cancelled || _failed || _finalEmitted) {
        return;
    }
    const auto staleConnectionFailure =
        failure.transportContext().has_value() || failure.protocolReason() == Http1FailureReason::PrematureEndOfStream;
    if (staleConnectionFailure && _leasedReused && !_staleRetried && !_responseObserved && canRetryStale()) {
        _retryStale = true;
        _staleRetried = true;
        return;
    }
    if (failure.transportContext()) {
        fail(*failure.transportContext());
        return;
    }
    auto context = NetworkErrorContext{"HTTP client request failed"_el, failure.description()};
    context.setRemoteEndpoint(_effectiveUrl.endpoint());
    switch (failure.kind()) {
    case Http1TransactionFailure::Kind::Timeout:
        context.setReason(NetworkErrorReason::Timeout);
        break;
    case Http1TransactionFailure::Kind::ResourceLimit:
        context.setReason(NetworkErrorReason::ResourceLimitExceeded);
        break;
    default:
        context.setReason(NetworkErrorReason::HttpProtocolFailure);
        break;
    }
    switch (failure.phase()) {
    case Http1TransactionFailure::Phase::Headers:
        context.setPhase(NetworkErrorPhase::HttpResponseHeaders);
        break;
    case Http1TransactionFailure::Phase::Body:
        context.setPhase(NetworkErrorPhase::HttpResponseBody);
        break;
    case Http1TransactionFailure::Phase::Closing:
        context.setPhase(NetworkErrorPhase::Closing);
        break;
    default:
        context.setPhase(NetworkErrorPhase::HttpRequest);
        break;
    }
    fail(std::move(context));
}

void HttpClientRequest::handleTransactionFinal() {
    _transactionFinalized = true;
    if (_response != nullptr && !_response->isFinal()) {
        return;
    }
    completeTransactionFinal();
}

void HttpClientRequest::handleResponseFinal() {
    if (_transactionFinalized && !_finalEmitted) {
        completeTransactionFinal();
    }
}

void HttpClientRequest::completeTransactionFinal() {
    if (_retryStale && !_failed) {
        _transaction.reset();
        _connection.reset();
        _response.reset();
        _fixedBodyOffset = {};
        _uploadFinished = false;
        _leasedReused = false;
        _responseObserved = false;
        _retryStale = false;
        _transactionFinalized = false;
        _state = NetworkSourceState::Starting;
        const auto self = std::static_pointer_cast<HttpClientRequest>(shared_from_this());
        ownerEvents()->invoke([self]() -> void { self->start(); });
        return;
    }
    auto returned = false;
    if (!_failed && _transaction != nullptr && _transaction->isConnectionReusable() && _connection != nullptr &&
        _connection->state() == ConnectionState::Active &&
        ConnectionProtocolAccess::takeRetainedInput(*_connection).isEmpty()) {
        _completedTransactions += unit::ItemCount::one();
        if (const auto session = _session.lock(); session && session->_state != NetworkSourceState::Closed &&
            session->_state != NetworkSourceState::Closing &&
            session->_configurationGeneration == _configurationGeneration) {
            session->_connectionManager->returnConnection(
                _connectionUrl, _configurationGeneration, _connection, _completedTransactions, _options);
            returned = true;
        }
    }
    if (_followingRedirect && !_failed) {
        if (!returned && _connection != nullptr && _connection->state() == ConnectionState::Active) {
            _connection->close();
        }
        startRedirectHop();
        return;
    }
    if (!returned && _connection != nullptr && _connection->state() == ConnectionState::Active) {
        _connection->close();
    }
    finishFinal();
}

auto HttpClientRequest::handleRedirect(const Http1DecodeEvent &event, const bool hasBody) -> bool {
    const auto status = event.response().status();
    if (status != HttpStatus::MovedPermanently && status != HttpStatus::Found && status != HttpStatus::SeeOther &&
        status != HttpStatus::TemporaryRedirect && status != HttpStatus::PermanentRedirect) {
        return false;
    }
    const auto location = event.response().headers().getFirst(HttpFieldType::Location);
    auto target = location.isEmpty() ? Url{} : _effectiveUrl.resolved(location);
    if (target.isValid() && !target.hasFragment() && _effectiveUrl.hasFragment()) {
        auto data = std::make_shared<UrlData>(*target._data);
        data->fragment = _effectiveUrl.fragment();
        data->hasFragment = true;
        target = Url{std::move(data)};
    }
    auto nextMethod = _hopMethod;
    auto discardBody = false;
    if ((status == HttpStatus::MovedPermanently || status == HttpStatus::Found) &&
        _hopMethod.standardType() == HttpMethodType::Post) {
        nextMethod = HttpMethodType::Get;
        discardBody = true;
    } else if (status == HttpStatus::SeeOther && _hopMethod.standardType() != HttpMethodType::Head) {
        nextMethod = HttpMethodType::Get;
        discardBody = true;
    }
    auto constraint = HttpClientRedirectConstraint::None;
    const auto canonical = target.isValid() ? canonicalRequestUrl(target) : text::String{};
    if (!target.isValid() || (target.scheme() != UrlScheme::Http && target.scheme() != UrlScheme::Https)) {
        constraint = HttpClientRedirectConstraint::InvalidTarget;
    } else if (_redirectCount >= _redirectOptions.maximumRedirects()) {
        constraint = HttpClientRedirectConstraint::RedirectLimit;
    } else if (std::ranges::find(_redirectHistory, canonical) != _redirectHistory.end()) {
        constraint = HttpClientRedirectConstraint::Loop;
    } else if (
        _effectiveUrl.scheme() == UrlScheme::Https && target.scheme() == UrlScheme::Http &&
        !_redirectOptions.allowsHttpsDowngrade()) {
        constraint = HttpClientRedirectConstraint::HttpsDowngrade;
    } else {
        const auto originalHost = HttpCookieJar::hostText(_url);
        const auto targetHost = HttpCookieJar::hostText(target);
        if (_redirectOptions.hostPolicy() == HttpClientRedirectHostPolicy::SameHost && originalHost != targetHost) {
            constraint = HttpClientRedirectConstraint::HostPolicy;
        } else if (_redirectOptions.hostPolicy() == HttpClientRedirectHostPolicy::SameRegistrableDomain) {
            const auto addressHost = _url.endpoint().host().isAddress() || target.endpoint().host().isAddress();
            const auto originalDomain = public_suffix::registrableDomain(originalHost);
            const auto targetDomain = public_suffix::registrableDomain(targetHost);
            if ((addressHost && originalHost != targetHost) ||
                (!addressHost &&
                    (originalDomain.isEmpty() || targetDomain.isEmpty() ? originalHost != targetHost
                                                                        : originalDomain != targetDomain))) {
                constraint = HttpClientRedirectConstraint::HostPolicy;
            }
        }
    }
    if (constraint == HttpClientRedirectConstraint::None && _bodyMode == BodyMode::Streaming && !discardBody) {
        constraint = HttpClientRedirectConstraint::BodyNotReplayable;
    }
    auto context = HttpClientRedirectContext{
        event.response(),
        _effectiveUrl,
        target.isValid() ? std::optional<Url>{target} : std::optional<Url>{},
        _redirectCount,
        constraint};
    auto action = _redirectOptions.action();
    if (_onRedirect) {
        action = _onRedirect(std::static_pointer_cast<HttpClientRequest>(shared_from_this()), context);
    }
    if (action == HttpClientRedirectAction::ReturnResponse) {
        return false;
    }
    if (action == HttpClientRedirectAction::Reject || constraint != HttpClientRedirectConstraint::None) {
        auto error = NetworkErrorContext{
            "HTTP redirect rejected"_el,
            action == HttpClientRedirectAction::Reject
                ? "The HTTP redirect policy rejected the response."_el
                : "A non-overridable HTTP redirect guard prevented following the target."_el};
        error.setReason(NetworkErrorReason::HttpRedirectFailure)
            .setPhase(NetworkErrorPhase::HttpResponseHeaders)
            .setRemoteEndpoint(_effectiveUrl.endpoint());
        fail(std::move(error));
        _transaction->cancel();
        return true;
    }
    if (context.requestOverride()) {
        const auto replacement = std::dynamic_pointer_cast<HttpClientRequest>(context.requestOverride());
        const auto session = _session.lock();
        if (replacement == nullptr || replacement->_session.lock().get() != session.get() || replacement->_submitted ||
            canonicalRequestUrl(replacement->_url) != canonical) {
            auto error = NetworkErrorContext{
                "Invalid HTTP redirect override"_el,
                "A redirect override must be a prepared request from the same session for the resolved target."_el};
            error.setReason(NetworkErrorReason::HttpRedirectFailure).setPhase(NetworkErrorPhase::HttpResponseHeaders);
            fail(std::move(error));
            _transaction->cancel();
            return true;
        }
        if (replacement->_bodyMode == BodyMode::Streaming ||
            replacement->_fixedBody.length() > _options.maximumBodyLength() ||
            !session->replaceBodyQuota(_retainedBodyLength, replacement->_fixedBody.length())) {
            auto error = NetworkErrorContext{
                "Invalid HTTP redirect override"_el,
                "A redirect override body must be replayable and fit the captured request limits."_el};
            error.setReason(NetworkErrorReason::HttpRedirectFailure).setPhase(NetworkErrorPhase::HttpResponseHeaders);
            fail(std::move(error));
            _transaction->cancel();
            return true;
        }
        nextMethod = replacement->_method;
        _hopHeaders = replacement->_headers;
        _fixedBody = replacement->_fixedBody;
        _bodyMode = replacement->_bodyMode;
        _retainedBodyLength = _fixedBody.length();
        discardBody = false;
    }
    if (!sameOrigin(_effectiveUrl, target)) {
        stripOriginBoundFields(_defaultHeaders);
        stripOriginBoundFields(_hopHeaders);
    }
    if (discardBody) {
        if (const auto session = _session.lock()) {
            session->releaseBodyQuota(_retainedBodyLength);
        }
        _retainedBodyLength = {};
        _bodyMode = BodyMode::Empty;
        _fixedBody = {};
        stripBodyFields(_defaultHeaders);
        stripBodyFields(_hopHeaders);
    }
    _hopMethod = std::move(nextMethod);
    _effectiveUrl = std::move(target);
    _redirectHistory.emplace_back(canonical);
    ++_redirectCount;
    _followingRedirect = true;
    if (hasBody) {
        _transaction->aggregateBody(_options.maximumBodyLength());
    }
    return true;
}

void HttpClientRequest::startRedirectHop() {
    _transaction.reset();
    _connection.reset();
    _response.reset();
    _fixedBodyOffset = {};
    _uploadFinished = false;
    _followingRedirect = false;
    _responseObserved = false;
    _transactionFinalized = false;
    _state = NetworkSourceState::Starting;
    start();
}

auto HttpClientRequest::canonicalRequestUrl(const Url &url) -> text::String {
    auto options = UrlFormatOptions{};
    options.setIncludeFragment(false);
    return url.toString(options);
}

auto HttpClientRequest::sameOrigin(const Url &first, const Url &second) noexcept -> bool {
    return first.scheme() == second.scheme() && first.endpoint() == second.endpoint();
}

void HttpClientRequest::stripOriginBoundFields(HttpHeaders &headers) {
    headers.removeAllFields(HttpFieldType::Authorization);
    headers.removeAllFields(HttpFieldType::ProxyAuthorization);
    headers.removeAllFields(HttpFieldType::Cookie);
    headers.removeAllFields(HttpFieldType::Referer);
    headers.removeAllFields("Origin"_el);
}

void HttpClientRequest::stripBodyFields(HttpHeaders &headers) {
    headers.removeAllFields(HttpFieldType::ContentEncoding);
    headers.removeAllFields(HttpFieldType::ContentLanguage);
    headers.removeAllFields(HttpFieldType::ContentLocation);
    headers.removeAllFields(HttpFieldType::ContentRange);
    headers.removeAllFields(HttpFieldType::ContentType);
}

auto HttpClientRequest::canRetryStale() const noexcept -> bool {
    if (_bodyMode == BodyMode::Streaming) {
        return false;
    }
    const auto type = _hopMethod.standardType();
    return type == HttpMethodType::Get || type == HttpMethodType::Head || type == HttpMethodType::Options ||
        type == HttpMethodType::Trace;
}

void HttpClientRequest::handleOverallTimeout(const std::uint64_t generation) {
    if (generation != _overallGeneration || _finalEmitted) {
        return;
    }
    auto context =
        NetworkErrorContext{"HTTP client request timed out"_el, "The overall HTTP client request deadline expired."_el};
    context.setReason(NetworkErrorReason::Timeout)
        .setPhase(NetworkErrorPhase::HttpRequest)
        .setRemoteEndpoint(_effectiveUrl.endpoint());
    fail(std::move(context));
    if (_transaction != nullptr) {
        _transaction->cancel();
    } else if (_connection != nullptr) {
        _connection->abort();
        finishFinal();
    } else {
        finishFinal();
    }
}

void HttpClientRequest::handlePhaseTimeout(const std::uint64_t generation, const NetworkErrorPhase phase) {
    if (generation != _phaseTimerGeneration || _finalEmitted || _state == NetworkSourceState::Active) {
        return;
    }
    auto context = NetworkErrorContext{
        "HTTP client connection timed out"_el,
        phase == NetworkErrorPhase::Resolving ? "The HTTP client DNS deadline expired."_el
                                              : "The HTTP client TCP connection deadline expired."_el};
    context.setReason(NetworkErrorReason::Timeout).setPhase(phase).setRemoteEndpoint(_effectiveUrl.endpoint());
    fail(std::move(context));
    if (_connection != nullptr) {
        _connection->abort();
    }
    finishFinal();
}

void HttpClientRequest::fail(NetworkErrorContext context) {
    if (_failed || _finalEmitted) {
        return;
    }
    _failed = true;
    _state = NetworkSourceState::Failed;
    if (_response != nullptr && !_response->isFinal()) {
        _response->finishFinal();
    }
    if (_onError) {
        try {
            _onError(std::static_pointer_cast<HttpClientRequest>(shared_from_this()), context);
        } catch (...) {}
    }
}

void HttpClientRequest::finishFinal() {
    if (_finalEmitted) {
        return;
    }
    _finalEmitted = true;
    ++_overallGeneration;
    ++_phaseTimerGeneration;
    if (!_failed) {
        _state = NetworkSourceState::Closed;
    }
    if (_onFinal) {
        try {
            _onFinal();
        } catch (...) {}
    }
    if (const auto session = _session.lock()) {
        session->requestFinalized(std::static_pointer_cast<HttpClientRequest>(shared_from_this()));
    }
    _transaction.reset();
    _connection.reset();
}

void HttpClientRequest::verifyPrepared() const {
    if (_submitted || _state != NetworkSourceState::Inactive) {
        throw err::LogicError{"An HTTP client request can only be prepared before submission."_el};
    }
}

void HttpClientRequest::validateSubmission() const {
    if (!_url._data || _url._data->hasUserInfo) {
        throw err::ParameterError{"HTTP client URL user-info is not supported."_el, "url"_el};
    }
    if (_fixedBody.length() > _options.maximumBodyLength()) {
        throw err::ParameterError{"The fixed HTTP request body exceeds the configured limit."_el, "body"_el};
    }
    const auto validateFields = [this](const HttpHeaders &headers) -> void {
        for (const auto &field : headers.fields()) {
            const auto type = field.name().type();
            if (type == HttpFieldType::Host || type == HttpFieldType::ContentLength ||
                type == HttpFieldType::TransferEncoding || type == HttpFieldType::Expect ||
                type == HttpFieldType::Upgrade) {
                throw err::ParameterError{
                    "Host, framing, Expect, and Upgrade fields are controlled by the HTTP client."_el, "headers"_el};
            }
            if (_options.automaticCookiesEnabled() && type == HttpFieldType::Cookie) {
                throw err::ParameterError{
                    "Cookie fields are controlled by the enabled HTTP client cookie jar."_el, "headers"_el};
            }
        }
    };
    validateFields(_defaultHeaders);
    validateFields(_headers);
    if (_method.standardType() == HttpMethodType::Connect) {
        throw err::ParameterError{"CONNECT is not supported by the HTTP/1.1 client."_el, "method"_el};
    }
    if (_responseHandler.kind() != HttpClientResponseHandler::Kind::Head &&
        (_responseHandler.options().maximumBodyLength().isZero() ||
            _responseHandler.options().maximumBodyLength().isInfinite() ||
            _responseHandler.options().maximumBodyLength() > _options.maximumBodyLength())) {
        throw err::ParameterError{
            "Automatic HTTP response aggregation must use a positive finite limit within the session bound."_el,
            "responseOptions"_el};
    }
}

auto HttpClientRequest::buildHeaders() const -> HttpHeaders {
    auto result = _defaultHeaders;
    auto replaced = std::vector<HttpFieldName>{};
    for (const auto &field : _hopHeaders.fields()) {
        if (std::ranges::find(replaced, field.name()) == replaced.end()) {
            result.removeAllFields(field.name());
            replaced.emplace_back(field.name());
        }
        result.addField(field);
    }
    result.setField(HttpFieldType::Host, hostFieldValue());
    if (_options.automaticCookiesEnabled()) {
        if (const auto session = _session.lock()) {
            const auto cookie = session->_cookieJar.requestCookieHeader(_effectiveUrl);
            if (!cookie.isEmpty()) {
                result.setField(HttpFieldType::Cookie, cookie);
            }
        }
    }
    if (_bodyMode == BodyMode::Fixed) {
        result.setField(HttpFieldType::ContentLength, text::String::fromInteger(_fixedBody.length().toRawValue()));
    } else if (_bodyMode == BodyMode::Streaming) {
        result.setField(HttpFieldType::TransferEncoding, "chunked"_el);
    }
    return result;
}

auto HttpClientRequest::requestTarget() const -> text::String {
    return UrlWriter{*_effectiveUrl._data, {}}.writeRequestTarget();
}

auto HttpClientRequest::hostFieldValue() const -> text::String {
    return UrlWriter{*_effectiveUrl._data, {}}.writeHostField();
}

}
