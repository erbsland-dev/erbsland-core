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

}
