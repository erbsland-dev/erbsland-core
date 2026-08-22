// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerRequest.hpp"

#include "HttpServerRequestEventEditor.hpp"
#include "HttpServerSession.hpp"

#include "../codec/Http1Transaction.hpp"

#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringEncoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../http/HttpFieldType.hpp"

#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpServerRequest::HttpServerRequest(
    event::EventsPtr ownerEvents,
    HttpRequestHead head,
    HttpRequestTarget target,
    ConnectionPtr connection,
    HttpRoutes::Parameters parameters,
    HttpHeaders responseFields,
    HttpServerOptions options,
    const bool hasBody,
    const bool suppressBody) :
    network::HttpServerRequest{std::move(ownerEvents)},
    _head{std::move(head)},
    _target{std::move(target)},
    _connection{std::move(connection)},
    _parameters{std::move(parameters)},
    _responseFields{std::move(responseFields)},
    _options{std::move(options)},
    _hasBody{hasBody},
    _suppressBody{suppressBody} {
}

HttpServerRequest::~HttpServerRequest() = default;

auto HttpServerRequest::head() const noexcept -> const HttpRequestHead & {
    return _head;
}
auto HttpServerRequest::path() const noexcept -> const String & {
    return _target.path();
}
auto HttpServerRequest::query() const noexcept -> const String & {
    return _target.query();
}

auto HttpServerRequest::parameter(const String &name) const -> std::optional<String> {
    for (const auto &[candidate, value] : _parameters) {
        if (candidate == name) {
            return value;
        }
    }
    return std::nullopt;
}

auto HttpServerRequest::localEndpoint() const -> std::optional<IpEndpoint> {
    return _connection->localEndpoint();
}
auto HttpServerRequest::remoteEndpoint() const -> std::optional<IpEndpoint> {
    return _connection->remoteEndpoint();
}
auto HttpServerRequest::connection() const noexcept -> const ConnectionPtr & {
    return _connection;
}
auto HttpServerRequest::session() const -> HttpServerSessionPtr {
    return _session;
}

void HttpServerRequest::streamBody() {
    verifyCurrentOwnerEvents();
    if (_final || _bodyPolicySelected || !_hasBody) {
        throw err::LogicError{"The request body policy cannot be changed in this state."_el};
    }
    _bodyPolicySelected = true;
    _transaction->streamBody();
}

void HttpServerRequest::aggregateBody(const unit::ByteLength maximumLength) {
    verifyCurrentOwnerEvents();
    if (_final || _bodyPolicySelected || !_hasBody) {
        throw err::LogicError{"The request body policy cannot be changed in this state."_el};
    }
    _bodyPolicySelected = true;
    _transaction->aggregateBody(maximumLength);
}

void HttpServerRequest::rejectBody() {
    verifyCurrentOwnerEvents();
    if (_final || _bodyPolicySelected || !_hasBody) {
        return;
    }
    _bodyPolicySelected = true;
    _transaction->rejectBody();
}

void HttpServerRequest::pauseBody() {
    verifyCurrentOwnerEvents();
    if (!_final && _transaction != nullptr) {
        _transaction->pauseBody();
    }
}

void HttpServerRequest::resumeBody() {
    verifyCurrentOwnerEvents();
    if (!_final && _transaction != nullptr) {
        _transaction->resumeBody();
    }
}

void HttpServerRequest::sendResponse(HttpResponseHead response, mem::ByteBlock body) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    if (_responseStarted) {
        throw err::LogicError{"An HTTP request response can only be committed once."_el};
    }
    if (body.length() > _options.maximumFixedResponseLength()) {
        throw err::ParameterError{"The fixed HTTP response exceeds the configured limit."_el, "body"_el};
    }
    response = prepareResponse(std::move(response), body.length());
    if (_hasBody && !_bodyPolicySelected) {
        _bodyPolicySelected = true;
    }
    _transaction->startResponse(std::move(response));
    _responseStarted = true;
    if (!_suppressBody && !body.isEmpty()) {
        const auto bodyStatus = _transaction->sendBody(body);
        if (bodyStatus.wouldBlock()) {
            _pendingFixedBody = std::move(body);
            return;
        }
        if (bodyStatus.isClosed()) {
            throw err::LogicError{"The HTTP response body cannot be submitted in this state."_el};
        }
    }
    _finishRequested = true;
    queueFinish({});
}

void HttpServerRequest::sendText(String body, const HttpStatus status, HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    fixedResponse(
        status,
        std::move(headers),
        StringEncoder{body}.encode(StringEncoding::Utf8, StringBomMode::Reject),
        "text/plain; charset=UTF-8"_el);
}

void HttpServerRequest::sendJson(String body, const HttpStatus status, HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    fixedResponse(
        status,
        std::move(headers),
        StringEncoder{body}.encode(StringEncoding::Utf8, StringBomMode::Reject),
        "application/json"_el);
}

void HttpServerRequest::sendJson(const text::json::JsonValue &body, const HttpStatus status, HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    sendJson(body.toString(), status, std::move(headers));
}

void HttpServerRequest::sendError(const HttpStatus status, String message) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    if (message.isEmpty()) {
        message = status.defaultReasonPhrase();
    }
    sendText(std::move(message), status, {});
}

void HttpServerRequest::sendRedirect(String location, const HttpStatus status, HttpHeaders headers) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    headers.setField(HttpFieldType::Location, std::move(location));
    sendResponse(HttpResponseHead{_head.version(), status, status.defaultReasonPhrase(), std::move(headers)}, {});
}

void HttpServerRequest::startResponse(HttpResponseHead response) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    if (_responseStarted) {
        throw err::LogicError{"An HTTP request response can only be committed once."_el};
    }
    auto headers = response.headers();
    verifyReservedCookie(headers);
    appendManagerResponseFields(headers);
    response = HttpResponseHead{response.version(), response.status(), response.reasonPhrase(), std::move(headers)};
    if (_hasBody && !_bodyPolicySelected) {
        _bodyPolicySelected = true;
    }
    _transaction->startResponse(std::move(response));
    _responseStarted = true;
    _streamingResponse = true;
}

auto HttpServerRequest::sendBody(const mem::ByteBlock &data) -> NetworkSendStatus {
    verifyCurrentOwnerEvents();
    if (_final) {
        return NetworkSendStatus::Closed;
    }
    if (!_responseStarted || !_streamingResponse || _finishRequested) {
        throw err::LogicError{"A streamed HTTP body cannot be sent in this response state."_el};
    }
    if (_suppressBody) {
        return NetworkSendStatus::Accepted;
    }
    return _transaction->sendBody(data);
}

void HttpServerRequest::finishBody(HttpHeaders trailers) {
    verifyCurrentOwnerEvents();
    if (_final) {
        return;
    }
    if (!_responseStarted || !_streamingResponse || _finishRequested) {
        throw err::LogicError{"A streamed HTTP response can only be finished once after it starts."_el};
    }
    _finishRequested = true;
    queueFinish(std::move(trailers));
}

auto HttpServerRequest::isResponseStarted() const noexcept -> bool {
    return _responseStarted;
}
auto HttpServerRequest::isFinal() const noexcept -> bool {
    return _final;
}

auto HttpServerRequest::events() -> network::HttpServerRequestEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpServerRequestEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

void HttpServerRequest::attach(Http1TransactionPtr transaction, HttpServerSessionPtr session) {
    _transaction = std::move(transaction);
    _session = std::move(session);
}

void HttpServerRequest::setSelection(
    HttpServerSessionPtr session,
    HttpHeaders responseFields,
    std::optional<text::String> reservedCookieName,
    HttpRoutes::Parameters parameters) {
    _session = std::move(session);
    _responseFields = std::move(responseFields);
    _reservedCookieName = std::move(reservedCookieName);
    _parameters = std::move(parameters);
}

void HttpServerRequest::deliverBody(mem::ByteBlock data) {
    if (_onBodyData) {
        _onBodyData(std::move(data));
    }
}
void HttpServerRequest::deliverAggregatedBody(mem::ByteBlock data) {
    if (_onBody) {
        _onBody(std::move(data));
    }
}
void HttpServerRequest::deliverTrailers(const HttpHeaders &trailers) {
    if (_onTrailers) {
        _onTrailers(trailers);
    }
}
void HttpServerRequest::deliverBodyCompleted() {
    if (_onBodyCompleted) {
        _onBodyCompleted();
    }
}
void HttpServerRequest::handleWritable() {
    if (_pendingFixedBody.has_value()) {
        const auto status = _transaction->sendBody(*_pendingFixedBody);
        if (status.wouldBlock()) {
            return;
        }
        if (status.isClosed()) {
            _transaction->cancel();
            return;
        }
        _pendingFixedBody.reset();
        _finishRequested = true;
        queueFinish({});
        return;
    }
    if (_pendingFinish.has_value()) {
        auto trailers = std::move(*_pendingFinish);
        _pendingFinish.reset();
        queueFinish(std::move(trailers));
        return;
    }
    if (!_finishRequested && _onWritable) {
        _onWritable();
    }
}

void HttpServerRequest::finalize() {
    if (_final) {
        return;
    }
    _final = true;
    _transaction.reset();
    if (_onFinal) {
        _onFinal();
    }
}

auto HttpServerRequest::prepareResponse(HttpResponseHead response, const unit::ByteLength bodyLength)
    -> HttpResponseHead {
    auto headers = response.headers();
    verifyReservedCookie(headers);
    headers.removeAllFields(HttpFieldType::TransferEncoding);
    headers.setField(HttpFieldType::ContentLength, String::fromInteger(bodyLength.toRawValue()));
    appendManagerResponseFields(headers);
    return HttpResponseHead{response.version(), response.status(), response.reasonPhrase(), std::move(headers)};
}

void HttpServerRequest::fixedResponse(
    const HttpStatus status, HttpHeaders headers, mem::ByteBlock body, String contentType) {
    headers.setField(HttpFieldType::ContentType, std::move(contentType));
    sendResponse(
        HttpResponseHead{_head.version(), status, status.defaultReasonPhrase(), std::move(headers)}, std::move(body));
}

void HttpServerRequest::queueFinish(HttpHeaders trailers) {
    const auto status = _transaction->finishBody(trailers);
    if (status.wouldBlock()) {
        _pendingFinish = std::move(trailers);
        return;
    }
    if (status.isClosed()) {
        throw err::LogicError{"The HTTP response cannot be finished in this state."_el};
    }
}

void HttpServerRequest::appendManagerResponseFields(HttpHeaders &headers) const {
    for (const auto &field : _responseFields.fields()) {
        headers.addField(field);
    }
    if (const auto session = std::dynamic_pointer_cast<HttpServerSession>(_session);
        session != nullptr && !session->isValid()) {
        for (const auto &field : session->invalidationResponseFields().fields()) {
            headers.addField(field);
        }
    }
}

void HttpServerRequest::verifyReservedCookie(const HttpHeaders &headers) const {
    if (!_reservedCookieName.has_value()) {
        return;
    }
    auto prefixEditor = StringEditor{};
    prefixEditor.append(*_reservedCookieName);
    prefixEditor.append(U'=');
    const auto prefix = String{prefixEditor};
    for (const auto &value : headers.getAll(HttpFieldType::SetCookie)) {
        if (value.startsWith(prefix)) {
            throw err::ParameterError{
                "The response attempts to set the session manager's reserved cookie."_el, "response"_el};
        }
    }
}

}
