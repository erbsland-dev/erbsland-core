// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "HttpClientResponse.hpp"

#include "HttpClientRequest.hpp"
#include "HttpClientResponseEventEditor.hpp"

#include "../codec/Http1Transaction.hpp"
#include "../HttpGrammar.hpp"

#include "../../../../data/json/JsonParseOptions.hpp"
#include "../../../../data/json/JsonValue.hpp"
#include "../../../../err/Exception.hpp"
#include "../../../../err/LogicFailure.hpp"
#include "../../../../err/ParseError.hpp"
#include "../../../../err/RuntimeError.hpp"
#include "../../../../event/Events.hpp"
#include "../../../../path/Path.hpp"
#include "../../../../path/PathCollisionMode.hpp"
#include "../../../../path/PathCommitOptions.hpp"
#include "../../../../path/PathError.hpp"
#include "../../../../path/PathInfo.hpp"
#include "../../../../path/PathOperations.hpp"
#include "../../../../path/PathTempFileOptions.hpp"
#include "../../../../stream/ByteOutputStream.hpp"
#include "../../../../stream/TempByteOutputStream.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/AsciiCategory.hpp"
#include "../../../../text/CaseSensitivity.hpp"
#include "../../../../text/EncodingError.hpp"
#include "../../../../text/EncodingMode.hpp"
#include "../../../../text/EscapeFormat.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../text/StringBomMode.hpp"
#include "../../../../text/StringCharReader.hpp"
#include "../../../../text/StringDecoder.hpp"
#include "../../../../text/StringEncoding.hpp"
#include "../../../../text/StringJoins.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../http/HttpFieldType.hpp"
#include "../../../http/HttpMediaType.hpp"
#include "../../../source/NetworkErrorContext.hpp"
#include "../../../source/NetworkErrorPhase.hpp"
#include "../../../source/NetworkErrorReason.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace erbsland::network::impl {

using namespace err;
using namespace path;
using namespace text;
using namespace text::literals;

void HttpClientResponse::sinkWriteCompleted(const std::exception_ptr failure) {
    const auto writtenLength = _pendingSinkBlock ? _pendingSinkBlock->length() : unit::ByteCount{};
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
        fail(NetworkErrorReason::ContentSinkFailed, "The HTTP response output stream failed."_el, failure);
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
    const auto continuation = _sinkCompletion;
    const auto weakSelf =
        std::weak_ptr<HttpClientResponse>{std::static_pointer_cast<HttpClientResponse>(shared_from_this())};
    _sinkWork.submit([weakSelf, continuation, destination, cancelled]() -> void {
        auto temporary = stream::TempByteOutputStreamPtr{};
        auto failure = std::exception_ptr{};
        try {
            if (cancelled->load()) {
                throw PathError{"The HTTP response path download was cancelled."_el};
            }
            const auto info = destination.info();
            if (info.exists() && !info.isRegularFile()) {
                throw PathError{"The HTTP response destination must be absent or a regular file."_el};
            }
            auto options = PathTempFileOptions{};
            options.setPrefix(".erbsland-http-"_el).setSuffix(".tmp"_el).setRemoveOnClose(true);
            temporary = destination.parent().operations().openTempByteOutputStreamOrThrow(options);
            if (cancelled->load()) {
                temporary->abort();
                throw PathError{"The HTTP response path download was cancelled."_el};
            }
        } catch (const RuntimeError &) {
            failure = std::current_exception();
        }
        continuation->invoke([weakSelf, temporary = std::move(temporary), failure]() mutable -> void {
            if (const auto self = weakSelf.lock()) {
                self->pathSinkOpened(std::move(temporary), failure);
            } else if (temporary != nullptr) {
                temporary->abort();
            }
        });
    });
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
        fail(
            NetworkErrorReason::ContentSinkFailed,
            text::join(
                "The HTTP response temporary file could not be created for destination '"_el,
                _destination->toString().toEscaped(EscapeFormat::Display),
                "'."_el),
            failure);
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
    const auto continuation = _sinkCompletion;
    const auto weakSelf =
        std::weak_ptr<HttpClientResponse>{std::static_pointer_cast<HttpClientResponse>(shared_from_this())};
    if (cancelled->load()) {
        _sinkPending = false;
        fail(NetworkErrorReason::ContentSinkFailed, "The HTTP response path commit was cancelled."_el);
        return;
    }
    _sinkWork.submit([weakSelf, continuation, temporary, destination, cancelled]() -> void {
        auto failure = std::exception_ptr{};
        try {
            if (cancelled->load()) {
                throw PathError{"The HTTP response path commit was cancelled."_el};
            }
            temporary->commitToOrThrow(
                destination, PathCommitOptions{}.setBeforeCommit([cancelled](const Path &, const Path &) -> void {
                    if (cancelled->load()) {
                        throw PathError{"The HTTP response path commit was cancelled."_el};
                    }
                }));
        } catch (const RuntimeError &) {
            failure = std::current_exception();
            temporary->abort();
        }
        continuation->invoke([weakSelf, failure]() -> void {
            if (const auto self = weakSelf.lock()) {
                self->pathSinkCommitted(failure);
            }
        });
    });
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
        fail(
            NetworkErrorReason::ContentSinkFailed, "The HTTP response destination could not be committed."_el, failure);
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
    } catch (const text::EncodingError &) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid UTF-8."_el);
        return;
    }
    if (handler.kind() == HttpClientResponseHandler::Kind::Text) {
        std::get<HttpClientTextResponseFn>(handler.callback())(request, self, std::move(textBody));
        return;
    }
    auto json = std::optional<data::json::JsonValue>{};
    try {
        auto options = data::json::JsonParseOptions{};
        options.setMaximumInputLength(handler.options().maximumBodyLength());
        json = data::json::JsonValue::fromStringOrThrow(textBody, options);
    } catch (const ParseError &) {
        fail(NetworkErrorReason::HttpResponseValidationFailure, "The HTTP response body is not valid JSON."_el);
        return;
    }
    std::get<HttpClientJsonResponseFn>(handler.callback())(request, self, std::move(*json));
}

auto HttpClientResponse::acceptsContentType(const HttpClientResponseHandler &handler) const -> bool {
    const auto &patterns = handler.options().acceptedContentTypes();
    if ((handler.kind() == HttpClientResponseHandler::Kind::Bytes && !patterns.has_value()) ||
        (patterns.has_value() && patterns->isEmpty())) {
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
        return reader.isAtEnd() && mediaType.subtype().endsWith(suffix, Char::compareAsciiFolded);
    }
    reader.startCapture();
    reader.advanceWhile(AsciiCategory::HttpToken);
    return reader.isAtEnd() && http_grammar::equalTokenCI(reader.takeCapture().toString(), mediaType.subtype());
}

void HttpClientResponse::fail(const NetworkErrorReason reason, text::String description, std::exception_ptr cause) {
    if (_final) {
        return;
    }
    const auto transaction = _transaction;
    if (const auto request = _request.lock()) {
        auto context = NetworkErrorContext{"HTTP response processing failed"_el, std::move(description)};
        context.setCause(std::move(cause))
            .setReason(reason)
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
        throw LogicFailure{"The HTTP response body policy was already selected or is final."_el};
    }
}

void HttpClientResponse::finishFinal() {
    if (_final) {
        return;
    }
    _final = true;
    _sinkCompletion.reset();
    _sinkWork = {};
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
        } catch (const RuntimeError &) {}
    }
    if (const auto request = _request.lock()) {
        ownerEvents()->invoke([request]() -> void { request->handleResponseFinal(); });
    }
}

}
