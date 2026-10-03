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
#include "../../../../unit/ByteCount.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteRange.hpp"

#include <algorithm>
#include <cstring>

namespace erbsland::network::impl {

using namespace unit;
using namespace text;
using namespace text::literals;

auto Http1Decoder::takeLine(const ByteCount limit, const Http1FailureReason limitReason) -> std::optional<String> {
    auto access = mem::impl::UnsafeRingBufferAccess{_input};
    const auto spans = access.readableSpans();
    const auto syntaxReason = _state == State::StartLine                ? Http1FailureReason::MalformedStartLine
        : (_state == State::ChunkLine || _state == State::ChunkDataEnd) ? Http1FailureReason::MalformedChunk
                                                                        : Http1FailureReason::MalformedField;
    auto index = ByteCount{};
    auto previousWasCr = false;
    for (const auto span : spans) {
        for (const auto byte : span) {
            const auto value = byte.toChar();
            if (value == '\n') {
                if (!previousWasCr) {
                    fail(syntaxReason, "Bare LF is forbidden in HTTP/1.x."_el);
                }
                const auto lineLength = index - ByteCount::one();
                if (lineLength > limit) {
                    fail(limitReason, "An HTTP protocol line exceeds its configured limit."_el);
                }
                if (lineLength == ByteCount{0U}) {
                    access.consumeRead(ByteCount{2U});
                    return String{};
                }
                auto buffer = text::impl::UnsafeU8StringBuffer{lineLength};
                auto offset = std::size_t{};
                auto remaining = lineLength;
                for (const auto source : spans) {
                    const auto count = std::min(ByteCount::fromSizeT(source.size()), remaining);
                    if (count > ByteCount{0U}) {
                        std::memcpy(buffer.data() + offset, source.data(), count.toSizeT());
                        offset += count.toSizeT();
                        remaining -= count;
                    }
                }
                access.consumeRead(lineLength + ByteCount{2U});
                return buffer.takeString(lineLength);
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

void Http1Decoder::parseRequestLine(const String &line) {
    static const auto cSpace = CharSet{U' '};
    const auto first = line.splitAtFirst(cSpace);
    if (!first || first->first.isEmpty()) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP request line is malformed."_el);
    }
    const auto second = first->second.splitAtFirst(cSpace);
    if (!second || second->first.isEmpty() || second->second.isEmpty() || second->second.containsOneOf(cSpace)) {
        fail(Http1FailureReason::MalformedStartLine, "A request line must contain exactly two spaces."_el);
    }
    _method = HttpMethod::fromString(first->first);
    _target = second->first;
    if (!_method.isValid() || !http_grammar::isRequestTarget(_target)) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP request control data is malformed."_el);
    }
    const auto &version = second->second;
    if (version == "HTTP/1.0"_el) {
        _version = HttpVersion::Http10;
    } else if (version == "HTTP/1.1"_el) {
        _version = HttpVersion::Http11;
    } else {
        fail(Http1FailureReason::UnsupportedVersion, "The HTTP version is unsupported."_el);
    }
}

void Http1Decoder::parseStatusLine(const String &line) {
    if (line.length() < ByteCount{13U} || line.charAt(ByteIndex{8U}) != U' ' || line.charAt(ByteIndex{12U}) != U' ') {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP status line is malformed."_el);
    }
    const auto version = line.slice(ByteRange{ByteIndex::zero(), ByteIndex{8U}});
    if (version == "HTTP/1.0"_el) {
        _version = HttpVersion::Http10;
    } else if (version == "HTTP/1.1"_el) {
        _version = HttpVersion::Http11;
    } else {
        fail(Http1FailureReason::UnsupportedVersion, "The HTTP version is unsupported."_el);
    }
    _status = HttpStatus::fromString(line.slice({ByteIndex{9U}, ByteIndex{12U}}));
    _reason = line.slice(ByteRange{ByteIndex{13U}, ByteCount::infinite()});
    if (!_status.isValid() || !http_grammar::isReasonPhrase(_reason)) {
        fail(Http1FailureReason::MalformedStartLine, "The HTTP response control data is malformed."_el);
    }
}

auto Http1Decoder::parseFieldLine(const String &line, const bool trailer) -> HttpField {
    static const auto cColon = CharSet{U':'};
    static const auto cOws = CharSet{U' ', U'\t'};
    const auto limits = trailer ? _limits.trailerLimits() : _limits.headerLimits();
    const auto limitReason =
        trailer ? Http1FailureReason::TrailerLimitExceeded : Http1FailureReason::HeaderLimitExceeded;
    if (line.isEmpty() || line.startsWith(" "_el) || line.startsWith("\t"_el)) {
        fail(Http1FailureReason::MalformedField, "Obsolete folded HTTP fields are forbidden."_el);
    }
    const auto parts = line.splitAtFirst(cColon);
    if (!parts || parts->first.isEmpty() || parts->first.charAt(StringSide::Back) == U' ' ||
        parts->first.charAt(StringSide::Back) == U'\t') {
        fail(Http1FailureReason::MalformedField, "An HTTP field line has an invalid colon delimiter."_el);
    }
    auto nameText = parts->first;
    auto value = parts->second.trimmed(cOws);
    if (nameText.length() > limits.maximumNameLength() || value.length() > limits.maximumValueLength()) {
        fail(limitReason, "An HTTP field component exceeds its configured limit."_el);
    }
    if (_fields.count().toSizeT() >= limits.maximumFieldCount().toSizeT()) {
        fail(limitReason, "An HTTP field section contains too many fields."_el);
    }
    const auto canonicalLength = nameText.length() + ByteCount{2U} + value.length() + ByteCount{2U};
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
            if (*value > _limits.maximumBodyLength()) {
                fail(Http1FailureReason::BodyLimitExceeded, "The declared HTTP body exceeds its configured limit."_el);
            }
            _contentLength = *value;
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

void Http1Decoder::accountBody(const ByteCount length) {
    if (length > _limits.maximumBodyLength() - std::min(_bodyDecoded, _limits.maximumBodyLength())) {
        fail(Http1FailureReason::BodyLimitExceeded, "Decoded body data exceeds its configured limit."_el);
    }
    _bodyDecoded += length;
}

void Http1Decoder::consumeInput(const ByteCount length) {
    auto access = mem::impl::UnsafeRingBufferAccess{_input};
    access.consumeRead(length);
}

auto Http1Decoder::takeInput(const ByteCount length) -> mem::ByteBlock {
    return _input.read(length);
}

void Http1Decoder::fail(const Http1FailureReason reason, const String &message) {
    _state = State::Failed;
    throw Http1ProtocolError{reason, message};
}

}
