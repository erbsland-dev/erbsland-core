// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1Decoder.hpp"

#include "Http1FramingParser.hpp"
#include "Http1ProtocolError.hpp"

#include "../HttpGrammar.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../mem/impl/UnsafeRingBufferAccess.hpp"
#include "../../../../text/AnyString.hpp"
#include "../../../../text/CharSet.hpp"
#include "../../../../text/impl/UnsafeU8StringBuffer.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteRange.hpp"

#include <algorithm>
#include <cstring>

namespace erbsland::network::impl {

using namespace text;
using namespace text::literals;

Http1Decoder::Http1Decoder(const Role role, const Http1CodecLimits limits) :
    _role{role},
    _limits{limits},
    _input{
        Http1CodecLimits::initialQueueCapacity(limits.maximumInputLength()),
        Http1CodecLimits::queueStorageLimit(limits.maximumInputLength())} {
}

auto Http1Decoder::feed(const mem::ConstByteSpan bytes) -> NetworkSendStatus {
    if (_state == State::Failed || _endOfInput || _state == State::Opaque) {
        return NetworkSendStatus::Closed;
    }
    const auto current = _input.length().toSizeT();
    const auto maximum = _limits.maximumInputLength().toSizeT();
    if (bytes.size() > maximum || current > maximum - bytes.size()) {
        return NetworkSendStatus::WouldBlock;
    }
    if (isFailure(_input.writeExact(bytes))) {
        return NetworkSendStatus::WouldBlock;
    }
    return NetworkSendStatus::Accepted;
}

void Http1Decoder::endOfInput() {
    if (_state == State::Failed || _endOfInput) {
        fail(Http1FailureReason::InvalidState, "HTTP decoder cannot accept EOF in its current state."_el);
    }
    _endOfInput = true;
}

auto Http1Decoder::next() -> std::optional<Http1DecodeEvent> {
    for (;;) {
        switch (_state) {
        case State::StartLine:
            return readStartLine();
        case State::Headers:
            return readHeaders();
        case State::FixedBody:
            return readFixedBody();
        case State::ChunkLine:
            if (auto event = readChunkLine()) {
                return event;
            }
            if (_state == State::ChunkLine) {
                return {};
            }
            break;
        case State::ChunkData:
            if (auto event = readChunkData()) {
                return event;
            }
            if (_state == State::ChunkData) {
                return {};
            }
            break;
        case State::ChunkDataEnd:
            if (auto event = readChunkDataEnd()) {
                return event;
            }
            if (_state == State::ChunkDataEnd) {
                return {};
            }
            break;
        case State::Trailers:
            return readTrailers();
        case State::CloseBody:
            return readCloseBody();
        case State::CompletePending:
        case State::OpaquePending:
            return emitCompletion();
        case State::Complete:
        case State::Opaque:
            return {};
        case State::Failed:
            fail(Http1FailureReason::InvalidState, "HTTP decoder is in a failed state."_el);
        }
    }
}

void Http1Decoder::reset() {
    if (_state != State::Complete) {
        fail(Http1FailureReason::InvalidState, "Only a completed HTTP decoder can be reset."_el);
    }
    _state = State::StartLine;
    _method = {};
    _target = {};
    _version = {};
    _status = {};
    _reason = {};
    _fields = {};
    _fieldBytes = {};
    _framing = Http1BodyFraming::None;
    _transferCoding = Http1TransferCoding::None;
    _contentLength.reset();
    _bodyRemaining = {};
    _bodyDecoded = {};
}

void Http1Decoder::setRequestMethod(HttpMethod method) {
    if (_role != Role::Response || _state != State::StartLine || !_fields.isEmpty()) {
        fail(Http1FailureReason::InvalidState, "Response method context cannot be changed now."_el);
    }
    _requestMethod = std::move(method);
}

auto Http1Decoder::takeOpaqueRemainder() -> mem::ByteBlock {
    if (_state != State::Opaque) {
        fail(Http1FailureReason::InvalidState, "Opaque data is only available after protocol switching."_el);
    }
    return _input.read(unit::ByteLength::infinite());
}

auto Http1Decoder::takeRetainedInput() -> mem::ByteBlock {
    if (_state != State::Complete) {
        fail(Http1FailureReason::InvalidState, "Retained input is only available after message completion."_el);
    }
    return _input.read(unit::ByteLength::infinite());
}

auto Http1Decoder::isComplete() const noexcept -> bool {
    return _state == State::Complete || _state == State::Opaque;
}

auto Http1Decoder::readStartLine() -> std::optional<Http1DecodeEvent> {
    const auto line = takeLine(_limits.maximumStartLineLength(), Http1FailureReason::StartLineTooLong);
    if (!line) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF before a complete HTTP start line."_el);
        }
        return {};
    }
    if (line->isEmpty()) {
        fail(Http1FailureReason::MalformedStartLine, "Leading blank lines are not accepted."_el);
    }
    if (_role == Role::Request) {
        parseRequestLine(*line);
    } else {
        parseStatusLine(*line);
    }
    _state = State::Headers;
    return readHeaders();
}

auto Http1Decoder::readHeaders() -> std::optional<Http1DecodeEvent> {
    const auto line =
        takeLine(_limits.headerLimits().maximumAggregateLength(), Http1FailureReason::HeaderLimitExceeded);
    if (!line) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF inside an HTTP header section."_el);
        }
        return {};
    }
    if (!line->isEmpty()) {
        _fields.append(parseFieldLine(*line, false));
        return readHeaders();
    }
    const auto headers = finishFields(false);
    selectFraming(headers);
    if (_role == Role::Request) {
        auto event = Http1DecodeEvent::requestHead(
            HttpRequestHead{_method, _target, _version, headers}, _framing, _contentLength);
        return event;
    }
    return Http1DecodeEvent::responseHead(
        HttpResponseHead{_version, _status, _reason, headers}, _framing, _contentLength);
}

auto Http1Decoder::readFixedBody() -> std::optional<Http1DecodeEvent> {
    if (_bodyRemaining.isZero()) {
        _state = State::CompletePending;
        return emitCompletion();
    }
    if (_input.isEmpty()) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF inside a fixed-length HTTP body."_el);
        }
        return {};
    }
    const auto length = unit::ByteLength{std::min(
        {_input.length().toSizeT(), _bodyRemaining.toSizeT(), Http1CodecLimits::cMaximumBodyChunkLength.toSizeT()})};
    accountBody(length);
    _bodyRemaining -= length;
    return Http1DecodeEvent::body(takeInput(length));
}

auto Http1Decoder::readChunkLine() -> std::optional<Http1DecodeEvent> {
    const auto line = takeLine(_limits.maximumChunkMetadataLength(), Http1FailureReason::ChunkMetadataTooLong);
    if (!line) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF before a complete chunk-size line."_el);
        }
        return {};
    }
    try {
        const auto value = Http1FramingParser::chunkSize(*line);
        if (value > _limits.maximumBodyLength().toSizeT() -
                std::min(_bodyDecoded.toSizeT(), _limits.maximumBodyLength().toSizeT())) {
            fail(Http1FailureReason::BodyLimitExceeded, "Chunked data exceeds the configured body limit."_el);
        }
        _bodyRemaining = unit::ByteLength{value};
    } catch (const err::ParseError &) {
        fail(Http1FailureReason::MalformedChunk, "The chunk-size metadata is malformed."_el);
    }
    if (_bodyRemaining.isZero()) {
        _fields = {};
        _fieldBytes = {};
        _state = State::Trailers;
    } else {
        _state = State::ChunkData;
    }
    return {};
}

auto Http1Decoder::readChunkData() -> std::optional<Http1DecodeEvent> {
    if (_bodyRemaining.isZero()) {
        _state = State::ChunkDataEnd;
        return readChunkDataEnd();
    }
    if (_input.isEmpty()) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF inside chunk data."_el);
        }
        return {};
    }
    const auto length = unit::ByteLength{std::min(
        {_input.length().toSizeT(), _bodyRemaining.toSizeT(), Http1CodecLimits::cMaximumBodyChunkLength.toSizeT()})};
    accountBody(length);
    _bodyRemaining -= length;
    return Http1DecodeEvent::body(takeInput(length));
}

auto Http1Decoder::readChunkDataEnd() -> std::optional<Http1DecodeEvent> {
    if (_input.length().toSizeT() < 2U) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF after chunk data."_el);
        }
        return {};
    }
    auto access = mem::impl::UnsafeRingBufferAccess{_input};
    const auto spans = access.readableSpans();
    const auto first = spans[0][0U].toChar();
    const auto second = spans[0].size() >= 2U ? spans[0][1U].toChar() : spans[1][0U].toChar();
    if (first != '\r' || second != '\n') {
        fail(Http1FailureReason::MalformedChunk, "Chunk data is not followed by CRLF."_el);
    }
    access.consumeRead(unit::ByteLength{2U});
    _state = State::ChunkLine;
    return {};
}

auto Http1Decoder::readTrailers() -> std::optional<Http1DecodeEvent> {
    const auto line =
        takeLine(_limits.trailerLimits().maximumAggregateLength(), Http1FailureReason::TrailerLimitExceeded);
    if (!line) {
        if (_endOfInput) {
            fail(Http1FailureReason::PrematureEndOfStream, "EOF inside an HTTP trailer section."_el);
        }
        return {};
    }
    if (!line->isEmpty()) {
        _fields.append(parseFieldLine(*line, true));
        return readTrailers();
    }
    auto trailers = finishFields(true);
    _state = State::CompletePending;
    return Http1DecodeEvent::trailers(std::move(trailers));
}

auto Http1Decoder::readCloseBody() -> std::optional<Http1DecodeEvent> {
    if (!_input.isEmpty()) {
        const auto length =
            unit::ByteLength{std::min(_input.length().toSizeT(), Http1CodecLimits::cMaximumBodyChunkLength.toSizeT())};
        accountBody(length);
        return Http1DecodeEvent::body(takeInput(length));
    }
    if (_endOfInput) {
        _state = State::CompletePending;
        return emitCompletion();
    }
    return {};
}

auto Http1Decoder::emitCompletion() -> Http1DecodeEvent {
    if (_state == State::OpaquePending) {
        _state = State::Opaque;
    } else {
        _state = State::Complete;
    }
    return Http1DecodeEvent::complete();
}

}
