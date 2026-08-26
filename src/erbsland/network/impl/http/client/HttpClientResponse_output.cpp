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
#include "../../../../text/AsciiCategory.hpp"
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
    reader.advanceWhile(AsciiCategory::HttpToken);
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
        reader.advanceWhile(AsciiCategory::HttpToken);
        auto suffix = StringEditor{};
        suffix.append(U'+').append(reader.takeCapture().toString());
        return reader.isAtEnd() && mediaType.subtype().endsWith(String{suffix}, Char::compareAsciiFolded);
    }
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
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
