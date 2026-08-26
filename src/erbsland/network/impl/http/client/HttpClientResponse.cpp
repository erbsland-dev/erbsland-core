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

}
