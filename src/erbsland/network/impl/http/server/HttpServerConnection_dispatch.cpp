// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpServerConnection.hpp"

#include "HttpRequestTarget.hpp"
#include "HttpServer.hpp"
#include "HttpServerRequest.hpp"
#include "HttpServerSession.hpp"

#include "../codec/Http1BodyFraming.hpp"
#include "../codec/Http1DecodeEvent.hpp"
#include "../codec/Http1Transaction.hpp"
#include "../codec/Http1TransactionFailure.hpp"
#include "../codec/Http1TransactionOptions.hpp"
#include "../HttpGrammar.hpp"

#include "../../../../err/Exception.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/EncodingMode.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringDecoder.hpp"
#include "../../../../text/StringEditor.hpp"
#include "../../../../text/StringEncoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpResponseHead.hpp"
#include "../../../http_server/HttpServerSessionContext.hpp"
#include "../../../http_server/HttpServerSessionManager.hpp"
#include "../../../Network.hpp"
#include "../../../source/ConnectionEventEditor.hpp"
#include "../../../tcp/TcpConnection.hpp"
#include "../../../tcp/TcpConnectionRequest.hpp"
#include "../../../tls/TlsServerConnection.hpp"

#include <algorithm>
#include <utility>

namespace erbsland::network::impl {
using namespace text;
using namespace text::literals;

void HttpServerConnection::dispatch(
    const std::shared_ptr<HttpServerRequest> &request, [[maybe_unused]] const Http1DecodeEvent &event) {
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    auto selection = HttpServerSessionSelection{};
    if (server->_sessionManager != nullptr) {
        const auto weakServer = std::weak_ptr<HttpServer>{server};
        auto context = HttpServerSessionContext{
            request,
            _secure,
            [weakServer](std::optional<String> identifier, HttpSessionDataPtr data) -> network::HttpServerSessionPtr {
                if (const auto target = weakServer.lock()) {
                    return target->createSession(std::move(identifier), std::move(data));
                }
                return {};
            }};
        selection = server->_sessionManager->selectSession(context);
    } else {
        if (_anonymousSession == nullptr || !_anonymousSession->isValid()) {
            _anonymousSession = server->createSession({}, {});
        }
        selection = HttpServerSessionSelection{_anonymousSession};
    }
    if (!selection.isValid() || !selection.session()->isValid()) {
        sendFrameworkError(HttpStatus::InternalServerError);
        return;
    }
    const auto concreteSession = std::dynamic_pointer_cast<impl::HttpServerSession>(selection.session());
    if (concreteSession == nullptr) {
        sendFrameworkError(HttpStatus::InternalServerError);
        return;
    }
    auto match = concreteSession->routes().match(request->head().method(), request->segments());
    if (!match.handler.isValid()) {
        auto serverMatch = server->_routes.match(request->head().method(), request->segments());
        match.pathMatched = match.pathMatched || serverMatch.pathMatched;
        for (const auto &method : serverMatch.allowed) {
            if (std::ranges::find(match.allowed, method) == match.allowed.end()) {
                match.allowed.emplace_back(method);
            }
        }
        if (serverMatch.handler.isValid()) {
            match.handler = std::move(serverMatch.handler);
            match.parameters = std::move(serverMatch.parameters);
            match.usedHeadFallback = serverMatch.usedHeadFallback;
        }
    }
    request->setSelection(
        selection.session(), selection.responseFields(), selection.reservedCookieName(), std::move(match.parameters));
    if (!match.handler.isValid()) {
        if (match.pathMatched) {
            auto allow = StringEditor{};
            for (const auto &method : match.allowed) {
                if (!allow.isEmpty()) {
                    allow.append(", "_el);
                }
                allow.append(method.toString());
            }
            auto headers = HttpHeaders{};
            if (!allow.isEmpty()) {
                headers.setField(HttpFieldType::Allow, String{allow});
            }
            request->sendResponse(
                HttpResponseHead{
                    request->head().version(),
                    HttpStatus::MethodNotAllowed,
                    HttpStatus{HttpStatus::MethodNotAllowed}.defaultReasonPhrase(),
                    std::move(headers)},
                {});
        } else {
            if (!server->_staticContent.empty()) {
                _staticContent = std::make_shared<HttpStaticContentOperation>(
                    server, request, server->_staticContent, server->_staticContentUse);
                _staticContent->start();
            } else {
                request->sendError(HttpStatus::NotFound, {});
            }
        }
        return;
    }
    _handler = std::move(match.handler);
    invokeRoute(_handler, event);
}

void HttpServerConnection::invokeRoute(const HttpRouteHandler &handler, const Http1DecodeEvent &event) {
    if (handler.kind() == HttpRouteHandler::Kind::Head) {
        const auto &callback = std::get<HttpServerRequestHeadFn>(handler.callback());
        invokeApplication([&]() -> void { callback(_request->session(), _request); });
        return;
    }
    const auto server = _server.lock();
    if (server == nullptr) {
        return;
    }
    const auto maximum = std::min(handler.options().maximumBodyLength(), server->_options.maximumBodyLength());
    if (event.contentLength().has_value() && *event.contentLength() > maximum) {
        _request->sendError(HttpStatus::ContentTooLarge, {});
        return;
    }
    if (!acceptsContentType(handler)) {
        _request->sendError(HttpStatus::UnsupportedMediaType, {});
        return;
    }
    if (!hasBody(event)) {
        handleAggregatedBody({});
        return;
    }
    _request->aggregateBody(maximum);
}

void HttpServerConnection::handleAggregatedBody(mem::ByteBlock body) {
    if (_request == nullptr || !_handler.isValid()) {
        return;
    }
    if (_handler.kind() == HttpRouteHandler::Kind::Bytes) {
        const auto callback = std::get<HttpServerRequestFn>(_handler.callback());
        invokeApplication([&]() mutable -> void { callback(_request->session(), _request, std::move(body)); });
        return;
    }
    auto textBody = text::String{};
    try {
        textBody = StringDecoder{body}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    } catch (...) {
        _request->sendError(HttpStatus::BadRequest, {});
        return;
    }
    if (_handler.kind() == HttpRouteHandler::Kind::Text) {
        const auto callback = std::get<HttpServerTextRequestFn>(_handler.callback());
        invokeApplication([&]() mutable -> void { callback(_request->session(), _request, std::move(textBody)); });
        return;
    }
    try {
        auto parseOptions = text::json::JsonParseOptions{};
        parseOptions.setMaximumInputLength(_handler.options().maximumBodyLength());
        auto jsonBody = text::json::JsonValue::fromStringOrThrow(textBody, parseOptions);
        const auto callback = std::get<HttpServerJsonRequestFn>(_handler.callback());
        invokeApplication([&]() mutable -> void { callback(_request->session(), _request, std::move(jsonBody)); });
    } catch (const err::Exception &) {
        _request->sendError(HttpStatus::BadRequest, {});
    }
}

void HttpServerConnection::invokeApplication(const std::function<void()> &callback) {
    try {
        callback();
    } catch (...) {
        if (_request != nullptr && !_request->isResponseStarted()) {
            _request->sendError(HttpStatus::InternalServerError, {});
        } else if (_transaction != nullptr) {
            _transaction->cancel();
        }
    }
}

auto HttpServerConnection::acceptsContentType(const HttpRouteHandler &handler) const -> bool {
    if (_request == nullptr || !_request->hasBody()) {
        return true;
    }
    const auto &explicitPatterns = handler.options().acceptedContentTypes();
    if ((handler.kind() == HttpRouteHandler::Kind::Bytes && !explicitPatterns.has_value()) ||
        (explicitPatterns.has_value() && explicitPatterns->empty())) {
        return true;
    }
    const auto values = _request->head().headers().getAll(HttpFieldType::ContentType);
    if (values.isEmpty()) {
        return !explicitPatterns.has_value() || explicitPatterns->empty();
    }
    if (values.count() != unit::ItemCount{1U}) {
        return false;
    }
    const auto mediaType = HttpMediaType::fromString(values.first());
    if (!mediaType.isValid()) {
        return false;
    }
    if (handler.kind() == HttpRouteHandler::Kind::Text || handler.kind() == HttpRouteHandler::Kind::Json) {
        if (const auto charset = mediaType.parameter("charset"_el);
            charset.has_value() && !http_grammar::equalTokenCI(*charset, "utf-8"_el)) {
            return false;
        }
    }
    if (explicitPatterns.has_value()) {
        return std::ranges::any_of(*explicitPatterns, [&mediaType](const String &pattern) -> bool {
            return matchesContentTypePattern(pattern, mediaType);
        });
    }
    if (handler.kind() == HttpRouteHandler::Kind::Bytes) {
        return true;
    }
    if (handler.kind() == HttpRouteHandler::Kind::Text) {
        return mediaType.type() == "text"_el;
    }
    return mediaType.type() == "application"_el &&
        (mediaType.subtype() == "json"_el || mediaType.subtype().endsWith("+json"_el, Char::compareAsciiFolded));
}

auto HttpServerConnection::matchesContentTypePattern(const String &pattern, const HttpMediaType &mediaType) -> bool {
    auto reader = StringCharReader{pattern};
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
    const auto type = reader.takeCapture().toString();
    reader.advanceIf(U'/');
    if (type == "*"_el) {
        return true;
    }
    if (!http_grammar::equalTokenCI(type, mediaType.type())) {
        return false;
    }
    if (reader.advanceIf(U'*')) {
        if (reader.isAtEnd()) {
            return true;
        }
        reader.advanceIf(U'+');
        reader.startCapture();
        reader.advanceWhile(AsciiCategory::HttpToken);
        auto suffix = StringEditor{};
        suffix.append(U'+').append(reader.takeCapture().toString());
        return mediaType.subtype().endsWith(String{suffix}, Char::compareAsciiFolded);
    }
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
    return http_grammar::equalTokenCI(reader.takeCapture().toString(), mediaType.subtype());
}

void HttpServerConnection::sendFrameworkError(const HttpStatus status) {
    if (_transaction == nullptr) {
        return;
    }
    if (_request != nullptr) {
        if (!_request->isResponseStarted()) {
            _request->sendError(status, {});
        }
        return;
    }
    auto headers = HttpHeaders{};
    headers.setField(HttpFieldType::ContentLength, "0"_el);
    _transaction->startResponse(
        HttpResponseHead{HttpVersion::Http11, status, status.defaultReasonPhrase(), std::move(headers)});
    (void)_transaction->finishBody();
}

auto HttpServerConnection::transactionOptions() const -> Http1TransactionOptions {
    const auto server = _server.lock();
    auto result = Http1TransactionOptions{};
    if (server == nullptr) {
        return result;
    }
    auto codec = Http1CodecLimits{};
    codec.setMaximumStartLineLength(server->_options.maximumStartLineLength())
        .setHeaderLimits(server->_options.headerLimits())
        .setTrailerLimits(server->_options.headerLimits())
        .setMaximumBodyLength(server->_options.maximumBodyLength())
        .setMaximumInputLength(server->_options.maximumQueueLength())
        .setMaximumOutputLength(server->_options.maximumQueueLength());
    result.setCodecLimits(codec)
        .setHeaderTimeout(server->_options.headerTimeout())
        .setBodyIdleTimeout(server->_options.bodyIdleTimeout())
        .setTotalTimeout(server->_options.totalTimeout())
        .setCloseTimeout(server->_options.closeTimeout());
    return result;
}

auto HttpServerConnection::hasBody(const Http1DecodeEvent &event) noexcept -> bool {
    if (event.framing() == Http1BodyFraming::Chunked) {
        return true;
    }
    return event.framing() == Http1BodyFraming::FixedLength && event.contentLength().has_value() &&
        !event.contentLength()->isZero();
}

auto HttpServerConnection::errorStatus(const Http1TransactionFailure &failure) noexcept -> HttpStatus {
    if (failure.kind() == Http1TransactionFailure::Kind::Timeout) {
        return HttpStatus::RequestTimeout;
    }
    if (failure.kind() == Http1TransactionFailure::Kind::ResourceLimit) {
        return failure.phase() == Http1TransactionFailure::Phase::Body ? HttpStatus::ContentTooLarge
                                                                       : HttpStatus::RequestHeaderFieldsTooLarge;
    }
    if (failure.phase() == Http1TransactionFailure::Phase::Headers) {
        return HttpStatus::BadRequest;
    }
    return HttpStatus::InternalServerError;
}

}
