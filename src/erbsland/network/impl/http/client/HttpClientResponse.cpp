// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientResponse.hpp"

#include "HttpClientRequest.hpp"
#include "HttpClientResponseEventEditor.hpp"

#include "../codec/Http1Transaction.hpp"
#include "../HttpGrammar.hpp"

#include "../../../../err/Exception.hpp"
#include "../../../../err/LogicError.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../path/Path.hpp"
#include "../../../../path/PathCollisionMode.hpp"
#include "../../../../path/PathInfo.hpp"
#include "../../../../path/PathMoveOptions.hpp"
#include "../../../../path/PathOperations.hpp"
#include "../../../../path/PathTempFileOptions.hpp"
#include "../../../../stream/ByteOutputStream.hpp"
#include "../../../../stream/TempByteOutputStream.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/CaseSensitivity.hpp"
#include "../../../../text/EncodingMode.hpp"
#include "../../../../text/json/JsonParseOptions.hpp"
#include "../../../../text/json/JsonValue.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringDecoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpMediaType.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorPhase.hpp"
#include "../../../source/NetworkErrorReason.hpp"
#include "../../ResolverService.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

HttpClientResponse::HttpClientResponse(
    std::shared_ptr<HttpClientRequest> request,
    HttpResponseHead head,
    Http1TransactionPtr transaction,
    const bool hasBody,
    Url effectiveUrl,
    const unit::ItemCount redirectCount,
    std::optional<unit::ByteLength> expectedLength) :
    network::HttpClientResponse{request->ownerEvents()},
    _request{std::move(request)},
    _head{std::move(head)},
    _effectiveUrl{std::move(effectiveUrl)},
    _redirectCount{redirectCount},
    _transaction{std::move(transaction)},
    _hasBody{hasBody},
    _expectedLength{expectedLength} {
}

HttpClientResponse::~HttpClientResponse() = default;

auto HttpClientResponse::head() const noexcept -> const HttpResponseHead & {
    return _head;
}

auto HttpClientResponse::trailers() const noexcept -> const HttpHeaders & {
    return _trailers;
}

auto HttpClientResponse::effectiveUrl() const noexcept -> const Url & {
    return _effectiveUrl;
}

auto HttpClientResponse::redirectCount() const noexcept -> unit::ItemCount {
    return _redirectCount;
}

void HttpClientResponse::streamBody() {
    verifyCurrentOwnerEvents();
    verifyAwaitingPolicy();
    _policySelected = true;
    if (_hasBody) {
        _transaction->streamBody();
    }
}

void HttpClientResponse::aggregateBody(const unit::ByteLength maximumLength) {
    verifyCurrentOwnerEvents();
    verifyAwaitingPolicy();
    if (maximumLength.isZero() || maximumLength.isInfinite()) {
        throw err::ParameterError{
            "An HTTP response aggregate limit must be positive and finite."_el, "maximumLength"_el};
    }
    _policySelected = true;
    _aggregateMode = AggregateMode::Bytes;
    if (_hasBody) {
        _transaction->aggregateBody(maximumLength);
    }
}

void HttpClientResponse::aggregateText(const unit::ByteLength maximumLength) {
    aggregateBody(maximumLength);
    _aggregateMode = AggregateMode::Text;
}

void HttpClientResponse::aggregateJson(const unit::ByteLength maximumLength) {
    aggregateBody(maximumLength);
    _aggregateMode = AggregateMode::Json;
}

void HttpClientResponse::writeBodyTo(stream::ByteOutputStreamPtr output) {
    verifyCurrentOwnerEvents();
    verifyAwaitingPolicy();
    if (output == nullptr || !output->isOpen()) {
        throw err::ParameterError{"An open byte output stream is required."_el, "output"_el};
    }
    _policySelected = true;
    _output = std::move(output);
    if (_hasBody) {
        _transaction->streamBody();
    }
}

void HttpClientResponse::writeBodyTo(path::Path destination) {
    verifyCurrentOwnerEvents();
    verifyAwaitingPolicy();
    if (destination.isEmpty() || !destination.isValid() || destination.parent().isEmpty()) {
        throw err::ParameterError{"A valid destination with a parent directory is required."_el, "destination"_el};
    }
    _policySelected = true;
    _destination = std::move(destination);
    _pathSinkCancelled = std::make_shared<std::atomic_bool>(false);
    _sinkPending = true;
    if (_hasBody) {
        _transaction->streamBody();
        _transaction->pauseBody();
    }
    openPathSink();
}

void HttpClientResponse::rejectBody() {
    verifyCurrentOwnerEvents();
    verifyAwaitingPolicy();
    _policySelected = true;
    if (_hasBody) {
        _transaction->rejectBody();
    }
}

void HttpClientResponse::pauseBody() {
    verifyCurrentOwnerEvents();
    if (_final || !_policySelected || _output != nullptr) {
        throw err::LogicError{"Only an active callback-streamed response body can be paused."_el};
    }
    _transaction->pauseBody();
}

void HttpClientResponse::resumeBody() {
    verifyCurrentOwnerEvents();
    if (_final || !_policySelected || _output != nullptr) {
        throw err::LogicError{"Only an explicitly paused callback-streamed response body can be resumed."_el};
    }
    _transaction->resumeBody();
}

auto HttpClientResponse::isFinal() const noexcept -> bool {
    return _final;
}

auto HttpClientResponse::events() -> network::HttpClientResponseEventEditor & {
    auto target = currentOwnerEvents();
    if (_eventEditor == nullptr) {
        _eventEditor = std::make_unique<HttpClientResponseEventEditor>(shared_from_this(), std::move(target), *this);
    }
    return *_eventEditor;
}

void HttpClientResponse::startAutomatic(const HttpClientResponseHandler &handler) {
    if (!acceptsContentType(handler)) {
        fail(
            NetworkErrorReason::HttpResponseValidationFailure,
            "The HTTP response Content-Type does not match the selected automatic handler."_el);
        return;
    }
    _automaticHandler = handler;
    _policySelected = true;
    switch (handler.kind()) {
    case HttpClientResponseHandler::Kind::Bytes:
        _aggregateMode = AggregateMode::Bytes;
        break;
    case HttpClientResponseHandler::Kind::Text:
        _aggregateMode = AggregateMode::Text;
        break;
    case HttpClientResponseHandler::Kind::Json:
        _aggregateMode = AggregateMode::Json;
        break;
    case HttpClientResponseHandler::Kind::Head:
        return;
    }
    if (_hasBody) {
        _transaction->aggregateBody(handler.options().maximumBodyLength());
    }
}

void HttpClientResponse::handleBodyData(mem::ByteBlock data) {
    if (_final) {
        return;
    }
    if (_output != nullptr) {
        _transaction->pauseBody();
        writeSinkBlock(std::move(data));
        return;
    }
    if (_onBodyData) {
        _onBodyData(std::move(data));
    }
}

void HttpClientResponse::handleAggregatedBody(mem::ByteBlock data) {
    if (_final) {
        return;
    }
    if (_automaticHandler) {
        deliverAutomatic(std::move(data));
        return;
    }
    if (_aggregateMode == AggregateMode::Bytes) {
        if (_onBody) {
            _onBody(std::move(data));
        }
        return;
    }
    auto textBody = text::String{};
    try {
        textBody = StringDecoder{data}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    } catch (...) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid UTF-8."_el);
        return;
    }
    if (_aggregateMode == AggregateMode::Text) {
        if (_onText) {
            _onText(std::move(textBody));
        }
        return;
    }
    auto json = std::optional<text::json::JsonValue>{};
    try {
        auto options = text::json::JsonParseOptions{};
        options.setMaximumInputLength(data.length());
        json = text::json::JsonValue::fromStringOrThrow(textBody, options);
    } catch (...) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid JSON."_el);
        return;
    }
    if (_onJson) {
        _onJson(std::move(*json));
    }
}

void HttpClientResponse::handleTrailers(const HttpHeaders &trailers) {
    _trailers = trailers;
    if (_onTrailers) {
        _onTrailers(_trailers);
    }
}

void HttpClientResponse::handleBodyCompleted() {
    _inputCompleted = true;
    if (_automaticHandler && !_hasBody) {
        deliverAutomatic({});
    } else if (!_automaticHandler && _policySelected && !_hasBody) {
        if (_aggregateMode == AggregateMode::Bytes && _onBody) {
            _onBody({});
        } else if (_aggregateMode == AggregateMode::Text && _onText) {
            _onText({});
        } else if (_aggregateMode == AggregateMode::Json) {
            fail(
                NetworkErrorReason::HttpResponseValidationFailure, "An empty HTTP response body is not valid JSON."_el);
            return;
        }
    }
    if (_sinkPending) {
        return;
    }
    if (_temporaryOutput != nullptr) {
        commitPathSink();
        return;
    }
    if (_onBodyCompleted) {
        _onBodyCompleted();
    }
    finishFinal();
}

void HttpClientResponse::writeSinkBlock(mem::ByteBlock data) {
    constexpr auto cMaximumSinkWriteLength = unit::ByteLength{16U * 1024U};
    if (data.length() > cMaximumSinkWriteLength) {
        _sinkRemainder =
            data.slice(unit::ByteIndex::end(cMaximumSinkWriteLength), data.length() - cMaximumSinkWriteLength);
        data = data.slice({}, cMaximumSinkWriteLength);
    }
    _sinkPending = true;
    _pendingSinkBlock = data;
    const auto output = _output;
    const auto weakSelf =
        std::weak_ptr<HttpClientResponse>{std::static_pointer_cast<HttpClientResponse>(shared_from_this())};
    const auto owner = ownerEvents();
    try {
        ResolverService::submit([weakSelf, owner, output, data = std::move(data)]() mutable -> void {
            auto failure = std::exception_ptr{};
            try {
                if (!output->write(data).isSuccess()) {
                    throw err::LogicError{"The HTTP response output stream timed out."_el};
                }
            } catch (...) {
                failure = std::current_exception();
            }
            owner->invoke([weakSelf, failure]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->sinkWriteCompleted(failure);
                }
            });
        });
    } catch (...) {
        sinkWriteCompleted(std::current_exception());
    }
}

void HttpClientResponse::sinkWriteCompleted(const std::exception_ptr failure) {
    const auto writtenLength = _pendingSinkBlock ? _pendingSinkBlock->length() : unit::ByteLength{};
    _sinkPending = false;
    _pendingSinkBlock.reset();
    if (_final) {
        if (_temporaryOutput != nullptr) {
            _temporaryOutput->abort();
            _temporaryOutput.reset();
            _output.reset();
        }
        _sinkRemainder.reset();
        return;
    }
    if (failure != nullptr) {
        fail(NetworkErrorReason::ContentSinkFailed, "The HTTP response output stream failed."_el);
        return;
    }
    _committedLength += writtenLength;
    if (_onBodyProgress) {
        _onBodyProgress(HttpClientBodyProgress{_committedLength, _expectedLength});
    }
    if (_final) {
        if (_temporaryOutput != nullptr) {
            _temporaryOutput->abort();
            _temporaryOutput.reset();
            _output.reset();
        }
        _sinkRemainder.reset();
        return;
    }
    if (_sinkRemainder.has_value()) {
        auto remainder = std::move(*_sinkRemainder);
        _sinkRemainder.reset();
        writeSinkBlock(std::move(remainder));
        return;
    }
    if (_inputCompleted) {
        if (_temporaryOutput != nullptr) {
            commitPathSink();
            return;
        }
        if (_onBodyCompleted) {
            _onBodyCompleted();
        }
        finishFinal();
    } else {
        _transaction->resumeBody();
    }
}

void HttpClientResponse::openPathSink() {
    const auto destination = *_destination;
    const auto cancelled = _pathSinkCancelled;
    const auto owner = ownerEvents();
    const auto weakSelf =
        std::weak_ptr<HttpClientResponse>{std::static_pointer_cast<HttpClientResponse>(shared_from_this())};
    try {
        ResolverService::submit([weakSelf, owner, destination, cancelled]() -> void {
            auto temporary = stream::TempByteOutputStreamPtr{};
            auto failure = std::exception_ptr{};
            try {
                if (cancelled->load()) {
                    throw err::LogicError{"The HTTP response path download was cancelled."_el};
                }
                const auto info = destination.info();
                if (info.exists() && !info.isRegularFile()) {
                    throw err::ParameterError{
                        "The HTTP response destination must be absent or a regular file."_el, "destination"_el};
                }
                auto options = path::PathTempFileOptions{};
                options.setPrefix(".erbsland-http-"_el).setSuffix(".tmp"_el).setRemoveOnClose(true);
                temporary = destination.parent().operations().openTempByteOutputStreamOrThrow(options);
                if (cancelled->load()) {
                    temporary->abort();
                    throw err::LogicError{"The HTTP response path download was cancelled."_el};
                }
            } catch (...) {
                failure = std::current_exception();
            }
            owner->invoke([weakSelf, temporary = std::move(temporary), failure]() mutable -> void {
                if (const auto self = weakSelf.lock()) {
                    self->pathSinkOpened(std::move(temporary), failure);
                } else if (temporary != nullptr) {
                    temporary->abort();
                }
            });
        });
    } catch (...) {
        pathSinkOpened({}, std::current_exception());
    }
}

void HttpClientResponse::pathSinkOpened(stream::TempByteOutputStreamPtr temporary, const std::exception_ptr failure) {
    _sinkPending = false;
    if (_final) {
        if (temporary != nullptr) {
            temporary->abort();
        }
        return;
    }
    if (failure != nullptr || temporary == nullptr) {
        fail(NetworkErrorReason::ContentSinkFailed, "The HTTP response temporary file could not be created."_el);
        return;
    }
    _temporaryOutput = std::move(temporary);
    _output = std::static_pointer_cast<stream::ByteOutputStream>(_temporaryOutput);
    if (_inputCompleted) {
        commitPathSink();
    } else {
        _transaction->resumeBody();
    }
}

void HttpClientResponse::commitPathSink() {
    _sinkPending = true;
    const auto temporary = _temporaryOutput;
    const auto destination = *_destination;
    const auto cancelled = _pathSinkCancelled;
    const auto owner = ownerEvents();
    const auto weakSelf =
        std::weak_ptr<HttpClientResponse>{std::static_pointer_cast<HttpClientResponse>(shared_from_this())};
    try {
        ResolverService::submit([weakSelf, owner, temporary, destination, cancelled]() -> void {
            auto failure = std::exception_ptr{};
            auto temporaryPath = path::Path{};
            try {
                if (cancelled->load()) {
                    throw err::LogicError{"The HTTP response path commit was cancelled."_el};
                }
                if (!temporary->flush().isSuccess()) {
                    throw err::LogicError{"The HTTP response temporary file could not be flushed."_el};
                }
                if (cancelled->load()) {
                    throw err::LogicError{"The HTTP response path commit was cancelled."_el};
                }
                temporary->setRemoveOnClose(false);
                if (!temporary->close().isClosed()) {
                    temporary->setRemoveOnClose(true);
                    throw err::LogicError{"The HTTP response temporary file could not be closed."_el};
                }
                temporaryPath = temporary->release();
                if (cancelled->load()) {
                    throw err::LogicError{"The HTTP response path commit was cancelled."_el};
                }
                auto options = path::PathMoveOptions{};
                options.setCollisionMode(path::PathCollisionMode::Overwrite);
                temporaryPath.operations().moveToOrThrow(destination, options);
                temporaryPath = {};
            } catch (...) {
                failure = std::current_exception();
                if (!temporaryPath.isEmpty()) {
                    temporaryPath.operations().remove();
                } else if (!temporary->isEmpty()) {
                    temporary->setRemoveOnClose(true);
                    temporary->abort();
                }
            }
            owner->invoke([weakSelf, failure]() -> void {
                if (const auto self = weakSelf.lock()) {
                    self->pathSinkCommitted(failure);
                }
            });
        });
    } catch (...) {
        pathSinkCommitted(std::current_exception());
    }
}

void HttpClientResponse::pathSinkCommitted(const std::exception_ptr failure) {
    _sinkPending = false;
    _output.reset();
    _temporaryOutput.reset();
    _pathSinkCancelled.reset();
    if (_final) {
        return;
    }
    if (failure != nullptr) {
        fail(NetworkErrorReason::ContentSinkFailed, "The HTTP response destination could not be committed."_el);
        return;
    }
    if (_onBodyCompleted) {
        _onBodyCompleted();
    }
    finishFinal();
}

void HttpClientResponse::deliverAutomatic(mem::ByteBlock data) {
    const auto handler = std::move(*_automaticHandler);
    _automaticHandler.reset();
    const auto request = _request.lock();
    if (request == nullptr || !handler.hasCallback()) {
        return;
    }
    const auto self = std::static_pointer_cast<HttpClientResponse>(shared_from_this());
    if (handler.kind() == HttpClientResponseHandler::Kind::Bytes) {
        std::get<HttpClientResponseFn>(handler.callback())(request, self, std::move(data));
        return;
    }
    auto textBody = text::String{};
    try {
        textBody = StringDecoder{data}.decode(StringEncoding::Utf8, StringBomMode::Reject, EncodingMode::Strict);
    } catch (...) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid UTF-8."_el);
        return;
    }
    if (handler.kind() == HttpClientResponseHandler::Kind::Text) {
        std::get<HttpClientTextResponseFn>(handler.callback())(request, self, std::move(textBody));
        return;
    }
    auto json = std::optional<text::json::JsonValue>{};
    try {
        auto options = text::json::JsonParseOptions{};
        options.setMaximumInputLength(handler.options().maximumBodyLength());
        json = text::json::JsonValue::fromStringOrThrow(textBody, options);
    } catch (const err::Exception &) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid JSON."_el);
        return;
    }
    std::get<HttpClientJsonResponseFn>(handler.callback())(request, self, std::move(*json));
}

auto HttpClientResponse::acceptsContentType(const HttpClientResponseHandler &handler) const -> bool {
    const auto &patterns = handler.options().acceptedContentTypes();
    if ((handler.kind() == HttpClientResponseHandler::Kind::Bytes && !patterns.has_value()) ||
        (patterns.has_value() && patterns->empty())) {
        return true;
    }
    const auto values = _head.headers().getAll(HttpFieldType::ContentType);
    if (values.isEmpty()) {
        return !patterns.has_value();
    }
    if (values.count() != unit::ItemCount::one()) {
        return false;
    }
    const auto mediaType = HttpMediaType::fromString(values.first());
    if (!mediaType.isValid()) {
        return false;
    }
    if (handler.kind() == HttpClientResponseHandler::Kind::Text ||
        handler.kind() == HttpClientResponseHandler::Kind::Json) {
        if (const auto charset = mediaType.parameter("charset"_el);
            charset.has_value() && !http_grammar::equalTokenCI(*charset, "utf-8"_el)) {
            return false;
        }
    }
    if (patterns.has_value()) {
        return std::ranges::any_of(*patterns, [&mediaType](const String &pattern) -> bool {
            return matchesContentTypePattern(pattern, mediaType);
        });
    }
    if (handler.kind() == HttpClientResponseHandler::Kind::Text) {
        return mediaType.type() == "text"_el;
    }
    return handler.kind() != HttpClientResponseHandler::Kind::Json ||
        (mediaType.type() == "application"_el &&
            (mediaType.subtype() == "json"_el || mediaType.subtype().endsWith("+json"_el, Char::compareAsciiFolded)));
}

auto HttpClientResponse::matchesContentTypePattern(const String &pattern, const HttpMediaType &mediaType) -> bool {
    auto reader = StringCharReader{pattern};
    reader.startCapture();
    reader.advanceWhile(http_grammar::tokenCharacters());
    const auto type = reader.takeCapture().toString();
    if (!reader.advanceIf(U'/')) {
        return false;
    }
    if (type != "*"_el && !http_grammar::equalTokenCI(type, mediaType.type())) {
        return false;
    }
    if (reader.advanceIf(U'*')) {
        if (reader.isAtEnd()) {
            return true;
        }
        if (!reader.advanceIf(U'+')) {
            return false;
        }
        reader.startCapture();
        reader.advanceWhile(http_grammar::tokenCharacters());
        auto suffix = StringEditor{};
        suffix.append(U'+').append(reader.takeCapture().toString());
        return reader.isAtEnd() && mediaType.subtype().endsWith(String{suffix}, Char::compareAsciiFolded);
    }
    reader.startCapture();
    reader.advanceWhile(http_grammar::tokenCharacters());
    return reader.isAtEnd() && http_grammar::equalTokenCI(reader.takeCapture().toString(), mediaType.subtype());
}

void HttpClientResponse::fail(const NetworkErrorReason reason, text::String description) {
    if (_final) {
        return;
    }
    const auto transaction = _transaction;
    if (const auto request = _request.lock()) {
        auto context = NetworkErrorContext{"HTTP response processing failed"_el, std::move(description)};
        context.setReason(reason)
            .setPhase(NetworkErrorPhase::HttpResponseBody)
            .setRemoteEndpoint(request->url().endpoint());
        request->fail(std::move(context));
    }
    if (transaction != nullptr) {
        transaction->cancel();
    }
    finishFinal();
}

void HttpClientResponse::verifyAwaitingPolicy() const {
    if (_final || _policySelected) {
        throw err::LogicError{"The HTTP response body policy was already selected or is final."_el};
    }
}

void HttpClientResponse::finishFinal() {
    if (_final) {
        return;
    }
    _final = true;
    if (_pathSinkCancelled != nullptr) {
        _pathSinkCancelled->store(true);
    }
    if (_temporaryOutput != nullptr && !_sinkPending) {
        _temporaryOutput->abort();
        _temporaryOutput.reset();
        _output.reset();
    }
    _transaction.reset();
    if (_onFinal) {
        try {
            _onFinal();
        } catch (...) {}
    }
    if (const auto request = _request.lock()) {
        ownerEvents()->invoke([request]() -> void { request->handleResponseFinal(); });
    }
}

}
