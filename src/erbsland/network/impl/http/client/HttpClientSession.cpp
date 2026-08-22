// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientSession.hpp"

#include "HttpClientRequest.hpp"
#include "HttpClientSessionEventEditor.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorPhase.hpp"
#include "../../../source/NetworkErrorReason.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {

using namespace text::literals;

HttpClientSession::HttpClientSession(event::EventsPtr ownerEvents) :
    network::HttpClientSession{ownerEvents},
    _cookieJar{ownerEvents},
    _connectionManager{std::make_shared<HttpClientConnectionManager>(std::move(ownerEvents))} {
}

HttpClientSession::~HttpClientSession() {
    for (const auto &request : _active) {
        request->cancel();
    }
    for (const auto &request : _pending) {
        request->cancel();
    }
}

void HttpClientSession::setOptions(HttpClientSessionOptions options) {
    verifyCurrentOwnerEvents();
    if (_state != NetworkSourceState::Active) {
        throw err::LogicError{"A closed HTTP client session cannot be configured."_el};
    }
    const auto previous = _options;
    _options = std::move(options);
    try {
        validateConfiguration();
        _cookieJar.setOptions(_options.cookieJarOptions());
    } catch (...) {
        _options = previous;
        throw;
    }
    ++_configurationGeneration;
    _connectionManager->invalidate();
    dispatch();
}

void HttpClientSession::setTlsOptions(HttpClientTlsOptions options) {
    verifyCurrentOwnerEvents();
    if (_state != NetworkSourceState::Active) {
        throw err::LogicError{"A closed HTTP client session cannot be configured."_el};
    }
    const auto previous = _tlsOptions;
    _tlsOptions = std::move(options);
    try {
        validateConfiguration();
    } catch (...) {
        _tlsOptions = previous;
        throw;
    }
    ++_configurationGeneration;
    _connectionManager->invalidate();
}

void HttpClientSession::setRedirectOptions(HttpClientRedirectOptions options) {
    verifyCurrentOwnerEvents();
    if (_state != NetworkSourceState::Active) {
        throw err::LogicError{"A closed HTTP client session cannot be configured."_el};
    }
    if (options.maximumRedirects().isZero() || options.maximumRedirects().isInfinite()) {
        throw err::ParameterError{"The redirect limit must be positive and finite."_el, "options"_el};
    }
    _redirectOptions = options;
}

void HttpClientSession::setDefaultHeaders(HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    if (_state != NetworkSourceState::Active) {
        throw err::LogicError{"A closed HTTP client session cannot be configured."_el};
    }
    for (const auto &field : headers.fields()) {
        const auto type = field.name().type();
        if (type == HttpFieldType::Host || type == HttpFieldType::ContentLength ||
            type == HttpFieldType::TransferEncoding || type == HttpFieldType::Expect ||
            type == HttpFieldType::Upgrade) {
            throw err::ParameterError{
                "Host, framing, Expect, and Upgrade fields are controlled by the HTTP client."_el, "headers"_el};
        }
    }
    _defaultHeaders = std::move(headers);
}

auto HttpClientSession::cookieJar() noexcept -> network::HttpCookieJar & {
    return _cookieJar;
}

auto HttpClientSession::createRequest(Url url) -> HttpClientRequestPtr {
    return createRequest(HttpMethodType::Get, std::move(url));
}

auto HttpClientSession::createRequest(HttpMethod method, Url url) -> HttpClientRequestPtr {
    verifyCurrentOwnerEvents();
    if (!method.isValid()) {
        throw err::ParameterError{"An HTTP client request requires a valid method."_el, "method"_el};
    }
    if (!url.isValid() || (url.scheme() != UrlScheme::Http && url.scheme() != UrlScheme::Https)) {
        throw err::ParameterError{"An HTTP client request requires an absolute HTTP or HTTPS URL."_el, "url"_el};
    }
    return std::make_shared<HttpClientRequest>(
        std::static_pointer_cast<HttpClientSession>(shared_from_this()), std::move(method), std::move(url));
}

void HttpClientSession::sendRequest(HttpClientRequestPtr request) {
    verifyCurrentOwnerEvents();
    const auto concrete = std::dynamic_pointer_cast<HttpClientRequest>(request);
    if (concrete == nullptr || concrete->_session.lock().get() != this) {
        throw err::ParameterError{"The request was not created by this HTTP client session."_el, "request"_el};
    }
    concrete->submit(
        _options,
        _tlsOptions,
        _redirectOptions,
        _defaultHeaders,
        _responseHandler,
        _onInformational,
        _onRedirect,
        _onError,
        _configurationGeneration);
    const auto bodyLength = concrete->_fixedBody.length();
    const auto retainedMaximum = _options.maximumRetainedRequestBodyLength().toRawValue();
    const auto bodyValue = bodyLength.toRawValue();
    const auto wouldOverflow = bodyValue > retainedMaximum - std::min(_retainedBodyLength, retainedMaximum);
    const auto queueFull = _pending.size() >= _options.maximumPendingRequests().toSizeT() &&
        _active.size() >= _options.maximumConcurrentRequests().toSizeT();
    if (_state != NetworkSourceState::Active || queueFull || wouldOverflow) {
        const auto closed = _state != NetworkSourceState::Active;
        ownerEvents()->invoke([target = std::move(concrete), closed]() -> void {
            auto context = NetworkErrorContext{
                closed ? "HTTP client session closed"_el : "HTTP client request limit"_el,
                closed ? "The HTTP client session no longer accepts requests."_el
                       : "The HTTP client request queue or retained-body quota is full."_el};
            context
                .setReason(closed ? NetworkErrorReason::ConfigurationFailed : NetworkErrorReason::ResourceLimitExceeded)
                .setPhase(NetworkErrorPhase::HttpRequest);
            target->failQueued(std::move(context));
        });
        return;
    }
    _retainedBodyLength += bodyValue;
    concrete->_retainedBodyLength = bodyLength;
    _pending.emplace_back(concrete);
    dispatch();
}

auto HttpClientSession::sendGet(Url url) -> HttpClientRequestPtr {
    auto request = createRequest(std::move(url));
    sendRequest(request);
    return request;
}

auto HttpClientSession::sendHead(Url url) -> HttpClientRequestPtr {
    auto request = createRequest(HttpMethodType::Head, std::move(url));
    sendRequest(request);
    return request;
}

auto HttpClientSession::sendPost(Url url, mem::ByteBlock body, HttpHeaders headers) -> HttpClientRequestPtr {
    auto request = createRequest(HttpMethodType::Post, std::move(url));
    request->setHeaders(std::move(headers));
    request->setBody(std::move(body));
    sendRequest(request);
    return request;
}

auto HttpClientSession::state() const noexcept -> NetworkSourceState {
    return _state;
}

void HttpClientSession::close() {
    verifyCurrentOwnerEvents();
    if (_state == NetworkSourceState::Closed || _state == NetworkSourceState::Closing) {
        return;
    }
    _state = NetworkSourceState::Closing;
    _connectionManager->invalidate();
    dispatch();
    assessClosed();
}

void HttpClientSession::abort() noexcept {
    if (_state == NetworkSourceState::Closed) {
        return;
    }
    _state = NetworkSourceState::Closed;
    _connectionManager->invalidate();
    const auto active = _active;
    const auto pending = _pending;
    for (const auto &request : active) {
        request->cancel();
    }
    for (const auto &request : pending) {
        request->cancel();
    }
    _active.clear();
    _pending.clear();
    _retainedBodyLength = 0U;
    finishFinal();
}

auto HttpClientSession::events() -> network::HttpClientSessionEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpClientSessionEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

void HttpClientSession::validateConfiguration() const {
    const auto positive = [](const time::TimeDelta value) -> bool { return value.isPositive(); };
    if (_options.maximumConcurrentRequests().isZero() || _options.maximumPendingRequests().isInfinite() ||
        _options.maximumRetainedRequestBodyLength().isInfinite() || _options.maximumBodyLength().isZero() ||
        _options.maximumBodyLength().isInfinite() || _options.maximumQueueLength().isZero() ||
        _options.maximumQueueLength().isInfinite() || _options.maximumStartLineLength().isZero() ||
        _options.maximumStartLineLength().isInfinite() || !positive(_options.dnsTimeout()) ||
        !positive(_options.connectTimeout()) || !positive(_options.headerTimeout()) ||
        !positive(_options.bodyIdleTimeout()) || !positive(_options.overallTimeout()) ||
        !positive(_options.closeTimeout()) || !positive(_options.idleConnectionTimeout()) ||
        _options.maximumIdleConnections().isInfinite() || _options.maximumTransactionsPerConnection().isZero() ||
        _options.maximumTransactionsPerConnection().isInfinite() ||
        _options.cookieJarOptions().maximumCookieLength().isZero() ||
        _options.cookieJarOptions().maximumCookieLength().isInfinite() ||
        _options.cookieJarOptions().maximumCookiesPerRegistrableDomain().isZero() ||
        _options.cookieJarOptions().maximumCookiesPerRegistrableDomain().isInfinite() ||
        _options.cookieJarOptions().maximumCookies().isZero() ||
        _options.cookieJarOptions().maximumCookies().isInfinite() || _tlsOptions.configurationLabel().isEmpty() ||
        !_tlsOptions.configurationLabel().isValidUtf8() || !positive(_tlsOptions.handshakeTimeout()) ||
        !positive(_tlsOptions.idleTimeout())) {
        throw err::ParameterError{"HTTP client limits and deadlines must be positive and finite."_el, "options"_el};
    }
}

void HttpClientSession::dispatch() {
    validateConfiguration();
    while (!_pending.empty() && _active.size() < _options.maximumConcurrentRequests().toSizeT()) {
        auto request = std::move(_pending.front());
        _pending.pop_front();
        _active.emplace_back(request);
        request->start();
    }
}

void HttpClientSession::requestFinalized(const std::shared_ptr<HttpClientRequest> &request) {
    auto found = false;
    const auto activeIt = std::ranges::find(_active, request);
    if (activeIt != _active.end()) {
        _active.erase(activeIt);
        found = true;
    }
    const auto pendingIt = std::ranges::find(_pending, request);
    if (pendingIt != _pending.end()) {
        _pending.erase(pendingIt);
        found = true;
    }
    if (found) {
        releaseBodyQuota(request->_retainedBodyLength);
    }
    dispatch();
    assessClosed();
}

void HttpClientSession::releaseBodyQuota(const unit::ByteLength length) noexcept {
    _retainedBodyLength -= std::min(_retainedBodyLength, length.toRawValue());
}

auto HttpClientSession::replaceBodyQuota(const unit::ByteLength oldLength, const unit::ByteLength newLength) noexcept
    -> bool {
    const auto oldValue = oldLength.toRawValue();
    const auto newValue = newLength.toRawValue();
    const auto retainedWithoutRequest = _retainedBodyLength - std::min(_retainedBodyLength, oldValue);
    const auto maximum = _options.maximumRetainedRequestBodyLength().toRawValue();
    if (newValue > maximum - std::min(retainedWithoutRequest, maximum)) {
        return false;
    }
    _retainedBodyLength = retainedWithoutRequest + newValue;
    return true;
}

void HttpClientSession::assessClosed() {
    if (_state != NetworkSourceState::Closing || !_pending.empty() || !_active.empty()) {
        return;
    }
    _state = NetworkSourceState::Closed;
    if (_onClosed) {
        try {
            _onClosed();
        } catch (...) {}
    }
    finishFinal();
}

void HttpClientSession::finishFinal() {
    if (_finalEmitted) {
        return;
    }
    _finalEmitted = true;
    if (_onFinal) {
        try {
            _onFinal();
        } catch (...) {}
    }
}

}
