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
