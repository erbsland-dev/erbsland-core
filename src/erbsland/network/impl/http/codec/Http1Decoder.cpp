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

auto Http1Decoder::takeLine(const unit::ByteLength limit, const Http1FailureReason limitReason)
    -> std::optional<text::String> {
    auto access = mem::impl::UnsafeRingBufferAccess{_input};
    const auto spans = access.readableSpans();
    const auto syntaxReason = _state == State::StartLine                ? Http1FailureReason::MalformedStartLine
        : (_state == State::ChunkLine || _state == State::ChunkDataEnd) ? Http1FailureReason::MalformedChunk
                                                                        : Http1FailureReason::MalformedField;
    auto index = std::size_t{};
    auto previousWasCr = false;
    for (const auto span : spans) {
        for (const auto byte : span) {
            const auto value = byte.toChar();
            if (value == '\n') {
                if (!previousWasCr) {
                    fail(syntaxReason, "Bare LF is forbidden in HTTP/1.x."_el);
                }
                const auto lineLength = index - 1U;
                if (lineLength > limit.toSizeT()) {
                    fail(limitReason, "An HTTP protocol line exceeds its configured limit."_el);
                }
                if (lineLength == 0U) {
                    access.consumeRead(unit::ByteLength{2U});
                    return text::String{};
                }
                auto buffer = text::impl::UnsafeU8StringBuffer{unit::ByteLength{lineLength}};
                auto offset = std::size_t{};
                auto remaining = lineLength;
                for (const auto source : spans) {
                    const auto count = std::min(source.size(), remaining);
                    if (count > 0U) {
                        std::memcpy(buffer.data() + offset, source.data(), count);
                        offset += count;
                        remaining -= count;
                    }
                }
                access.consumeRead(unit::ByteLength{lineLength + 2U});
                return buffer.takeString(unit::ByteLength{lineLength});
            }
            if (previousWasCr) {
                fail(syntaxReason, "Bare CR is forbidden in HTTP/1.x."_el);
            }
            previousWasCr = value == '\r';
            ++index;
        }
    }
    if (_input.length().toSizeT() > limit.toSizeT() + 1U) {
        fail(limitReason, "An HTTP protocol line exceeds its configured limit."_el);
    }
    return {};
}

void Http1Decoder::parseRequestLine(const text::String &line) {
    static const auto cSpace = CharSet{U' '};
    const auto firstSpace = line.findFirstOf(cSpace);
    if (firstSpace.isNoIndex() || firstSpace.isZero()) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP request line is malformed."_el);
    }
    const auto secondSpace = line.findFirstOf(cSpace, firstSpace.incremented());
    if (secondSpace.isNoIndex() || secondSpace == firstSpace.incremented() ||
        secondSpace.incremented() >= unit::ByteIndex::end(line.length()) ||
        !line.findFirstOf(cSpace, secondSpace.incremented()).isNoIndex()) {
        fail(Http1FailureReason::MalformedStartLine, "A request line must contain exactly two spaces."_el);
    }
    _method = HttpMethod::fromString(line.slice(unit::ByteRange{unit::ByteIndex::zero(), firstSpace}));
    _target = line.slice(unit::ByteRange{firstSpace.incremented(), secondSpace});
    if (!_method.isValid() || !http_grammar::isRequestTarget(_target)) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP request control data is malformed."_el);
    }
    const auto version = line.slice(unit::ByteRange{secondSpace.incremented(), unit::ByteLength::infinite()});
    if (version == "HTTP/1.0"_el) {
        _version = HttpVersion::Http10;
    } else if (version == "HTTP/1.1"_el) {
        _version = HttpVersion::Http11;
    } else {
        fail(Http1FailureReason::UnsupportedVersion, "The HTTP version is unsupported."_el);
    }
}

void Http1Decoder::parseStatusLine(const text::String &line) {
    if (line.length().toSizeT() < 13U || line.charAt(unit::ByteIndex{8U}) != U' ' ||
        line.charAt(unit::ByteIndex{12U}) != U' ') {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP status line is malformed."_el);
    }
    const auto version = line.slice(unit::ByteRange{unit::ByteIndex::zero(), unit::ByteIndex{8U}});
    if (version == "HTTP/1.0"_el) {
        _version = HttpVersion::Http10;
    } else if (version == "HTTP/1.1"_el) {
        _version = HttpVersion::Http11;
    } else {
        fail(Http1FailureReason::UnsupportedVersion, "The HTTP version is unsupported."_el);
    }
    _status = HttpStatus::fromString(line.slice(unit::ByteRange{unit::ByteIndex{9U}, unit::ByteIndex{12U}}));
    _reason = line.slice(unit::ByteRange{unit::ByteIndex{13U}, unit::ByteLength::infinite()});
    if (!_status.isValid() || !http_grammar::isReasonPhrase(_reason)) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP response control data is malformed."_el);
    }
}

auto Http1Decoder::parseFieldLine(const text::String &line, const bool trailer) -> HttpField {
    static const auto cColon = CharSet{U':'};
    static const auto cOws = CharSet{U' ', U'\t'};
    const auto limits = trailer ? _limits.trailerLimits() : _limits.headerLimits();
    const auto limitReason =
        trailer ? Http1FailureReason::TrailerLimitExceeded : Http1FailureReason::HeaderLimitExceeded;
    if (line.isEmpty() || line.startsWith(" "_el) || line.startsWith("\t"_el)) {
        fail(Http1FailureReason::MalformedField, "Obsolete folded HTTP fields are forbidden."_el);
    }
    const auto colon = line.findFirstOf(cColon);
    if (colon.isNoIndex() || colon.isZero() || line.charAt(colon.retreated(unit::ByteLength::one())) == U' ' ||
        line.charAt(colon.retreated(unit::ByteLength::one())) == U'\t') {
        fail(Http1FailureReason::MalformedField, "An HTTP field line has an invalid colon delimiter."_el);
    }
    auto nameText = line.slice(unit::ByteRange{unit::ByteIndex::zero(), colon});
    auto value = line.slice(unit::ByteRange{colon.incremented(), unit::ByteLength::infinite()}).trimmed(cOws);
    if (nameText.length() > limits.maximumNameLength() || value.length() > limits.maximumValueLength()) {
        fail(limitReason, "An HTTP field component exceeds its configured limit."_el);
    }
    if (_fields.count().toSizeT() >= limits.maximumFieldCount().toSizeT()) {
        fail(limitReason, "An HTTP field section contains too many fields."_el);
    }
    const auto canonicalLength = nameText.length() + unit::ByteLength{2U} + value.length() + unit::ByteLength{2U};
    const auto maximumAggregate = limits.maximumAggregateLength().toSizeT();
    if (_fieldBytes.toSizeT() > maximumAggregate ||
        canonicalLength.toSizeT() > maximumAggregate - _fieldBytes.toSizeT()) {
        fail(limitReason, "An HTTP field section exceeds its aggregate limit."_el);
    }
    _fieldBytes += canonicalLength;
    auto name = HttpFieldName::fromString(std::move(nameText));
    if (!name.isValid() || !http_grammar::isFieldValue(value)) {
        fail(Http1FailureReason::MalformedField, "An HTTP field contains invalid wire bytes."_el);
    }
    if (trailer && name.type().isKnown()) {
        fail(Http1FailureReason::ForbiddenTrailer, "A recognized HTTP field is forbidden in trailers."_el);
    }
    return HttpField{std::move(name), std::move(value)};
}

auto Http1Decoder::finishFields(const bool trailer) -> HttpHeaders {
    auto result = HttpHeaders{std::move(_fields), trailer ? _limits.trailerLimits() : _limits.headerLimits()};
    _fields = {};
    _fieldBytes = {};
    return result;
}

void Http1Decoder::selectFraming(const HttpHeaders &headers) {
    try {
        const auto parser = Http1FramingParser{headers};
        if (const auto value = parser.contentLength()) {
            if (*value > _limits.maximumBodyLength().toSizeT()) {
                fail(Http1FailureReason::BodyLimitExceeded, "The declared HTTP body exceeds its configured limit."_el);
            }
            _contentLength = unit::ByteLength{*value};
        } else {
            _contentLength.reset();
        }
        _transferCoding = parser.transferCoding();
    } catch (const err::ParseError &) {
        fail(Http1FailureReason::AmbiguousFraming, "HTTP message framing is malformed or ambiguous."_el);
    }
    if (_contentLength && _transferCoding != Http1TransferCoding::None) {
        fail(Http1FailureReason::AmbiguousFraming, "Content-Length and Transfer-Encoding cannot be combined."_el);
    }
    if (_role == Role::Request) {
        selectRequestFraming();
    } else {
        selectResponseFraming();
    }
}

void Http1Decoder::selectRequestFraming() {
    if (_transferCoding != Http1TransferCoding::None) {
        if (_version == HttpVersion::Http10 || _transferCoding != Http1TransferCoding::Chunked) {
            fail(Http1FailureReason::UnsupportedTransferCoding, "The request transfer coding is unsupported."_el);
        }
        _framing = Http1BodyFraming::Chunked;
        _state = State::ChunkLine;
    } else if (_contentLength) {
        _framing = Http1BodyFraming::FixedLength;
        _bodyRemaining = *_contentLength;
        _state = _bodyRemaining.isZero() ? State::CompletePending : State::FixedBody;
    } else {
        _framing = Http1BodyFraming::None;
        _state = State::CompletePending;
    }
}

void Http1Decoder::selectResponseFraming() {
    const auto methodType = _requestMethod.standardType();
    const auto isHead = methodType && *methodType == HttpMethodType::Head;
    const auto isConnect = methodType && *methodType == HttpMethodType::Connect;
    if (_status == HttpStatus::SwitchingProtocols || (isConnect && _status.isSuccessful())) {
        _framing = Http1BodyFraming::Opaque;
        _state = State::OpaquePending;
        return;
    }
    if (_transferCoding != Http1TransferCoding::None &&
        (_status.isInformational() || _status == HttpStatus::NoContent)) {
        fail(Http1FailureReason::AmbiguousFraming, "Transfer-Encoding is forbidden for this response status."_el);
    }
    if (_contentLength && (_status.isInformational() || _status == HttpStatus::NoContent)) {
        fail(Http1FailureReason::AmbiguousFraming, "Content-Length is forbidden for this response status."_el);
    }
    if (isHead || _status.isInformational() || _status == HttpStatus::NoContent || _status == HttpStatus::NotModified) {
        _framing = Http1BodyFraming::None;
        _state = State::CompletePending;
        return;
    }
    if (_transferCoding != Http1TransferCoding::None) {
        if (_version == HttpVersion::Http10 || _transferCoding != Http1TransferCoding::Chunked) {
            fail(Http1FailureReason::UnsupportedTransferCoding, "The response transfer coding is unsupported."_el);
        }
        _framing = Http1BodyFraming::Chunked;
        _state = State::ChunkLine;
    } else if (_contentLength) {
        _framing = Http1BodyFraming::FixedLength;
        _bodyRemaining = *_contentLength;
        _state = _bodyRemaining.isZero() ? State::CompletePending : State::FixedBody;
    } else {
        _framing = Http1BodyFraming::CloseDelimited;
        _state = State::CloseBody;
    }
}

void Http1Decoder::accountBody(const unit::ByteLength length) {
    if (length.toSizeT() > _limits.maximumBodyLength().toSizeT() -
            std::min(_bodyDecoded.toSizeT(), _limits.maximumBodyLength().toSizeT())) {
        fail(Http1FailureReason::BodyLimitExceeded, "Decoded body data exceeds its configured limit."_el);
    }
    _bodyDecoded += length;
}

void Http1Decoder::consumeInput(const unit::ByteLength length) {
    auto access = mem::impl::UnsafeRingBufferAccess{_input};
    access.consumeRead(length);
}

auto Http1Decoder::takeInput(const unit::ByteLength length) -> mem::ByteBlock {
    return _input.read(length);
}

void Http1Decoder::fail(const Http1FailureReason reason, const text::String &message) {
    _state = State::Failed;
    throw Http1ProtocolError{reason, message};
}

}
