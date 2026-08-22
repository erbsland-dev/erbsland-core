// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Http1Encoder.hpp"

#include "Http1FramingParser.hpp"
#include "Http1ProtocolError.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <exception>

namespace erbsland::network::impl {

using namespace text::literals;

Http1Encoder::Http1Encoder(HttpRequestHead head, const Http1CodecLimits limits) :
    _limits{limits},
    _output{
        Http1CodecLimits::initialQueueCapacity(limits.maximumOutputLength()),
        Http1CodecLimits::queueStorageLimit(limits.maximumOutputLength())} {
    if (!head.isValid()) {
        misuse("Cannot encode an invalid HTTP request head."_el);
    }
    if (head.method().text().length() + head.target().length() + unit::ByteLength{10U} >
        _limits.maximumStartLineLength()) {
        misuse("The HTTP request start line exceeds its configured limit."_el);
    }
    validateFields(head.headers(), _limits.headerLimits());
    selectRequestFraming(head);
    beginRequest(head);
}

Http1Encoder::Http1Encoder(HttpResponseHead head, HttpMethod requestMethod, const Http1CodecLimits limits) :
    _limits{limits},
    _output{
        Http1CodecLimits::initialQueueCapacity(limits.maximumOutputLength()),
        Http1CodecLimits::queueStorageLimit(limits.maximumOutputLength())} {
    if (!head.isValid()) {
        misuse("Cannot encode an invalid HTTP response head."_el);
    }
    if (head.reasonPhrase().length() + unit::ByteLength{13U} > _limits.maximumStartLineLength()) {
        misuse("The HTTP response start line exceeds its configured limit."_el);
    }
    validateFields(head.headers(), _limits.headerLimits());
    selectResponseFraming(head, requestMethod);
    beginResponse(head, requestMethod);
}

auto Http1Encoder::writeBody(const mem::ConstByteSpan bytes) -> NetworkSendStatus {
    if (_complete || _framing == Http1BodyFraming::None || _framing == Http1BodyFraming::Opaque) {
        return NetworkSendStatus::Closed;
    }
    if (bytes.size() > Http1CodecLimits::cMaximumBodyChunkLength.toSizeT()) {
        misuse("An HTTP encoder body fragment exceeds 16 KiB."_el);
    }
    if (bytes.size() > _limits.maximumBodyLength().toSizeT() -
            std::min(_bodyWritten.toSizeT(), _limits.maximumBodyLength().toSizeT())) {
        misuse("The encoded HTTP body exceeds its configured limit."_el);
    }
    if (_framing == Http1BodyFraming::FixedLength && bytes.size() > _bodyRemaining.toSizeT()) {
        misuse("Too many body bytes were supplied for Content-Length."_el);
    }
    if (_framing == Http1BodyFraming::Chunked && bytes.empty()) {
        return NetworkSendStatus::Accepted;
    }
    const auto required = _framing == Http1BodyFraming::Chunked ? chunkWireLength(unit::ByteLength{bytes.size()})
                                                                : unit::ByteLength{bytes.size()};
    if (!canQueue(required) || isFailure(_output.reserveAdditional(required))) {
        return NetworkSendStatus::WouldBlock;
    }
    if (_framing == Http1BodyFraming::Chunked) {
        appendChunkPrefix(unit::ByteLength{bytes.size()});
        if (isFailure(_output.writeExact(bytes))) {
            std::terminate();
        }
        appendLiteral("\r\n");
    } else {
        if (isFailure(_output.writeExact(bytes))) {
            std::terminate();
        }
    }
    const auto length = unit::ByteLength{bytes.size()};
    _bodyWritten += length;
    if (_framing == Http1BodyFraming::FixedLength) {
        _bodyRemaining -= length;
    }
    return NetworkSendStatus::Accepted;
}

auto Http1Encoder::finish(HttpHeaders trailers) -> NetworkSendStatus {
    if (_complete) {
        return NetworkSendStatus::Closed;
    }
    if (_framing != Http1BodyFraming::Chunked && !trailers.fieldCount().isZero()) {
        misuse("HTTP trailers require chunked transfer coding."_el);
    }
    if (_framing == Http1BodyFraming::FixedLength && !_bodyRemaining.isZero()) {
        misuse("Fewer body bytes than Content-Length were supplied."_el);
    }
    const auto required = _framing == Http1BodyFraming::Chunked ? trailerWireLength(trailers) + unit::ByteLength{5U}
                                                                : unit::ByteLength::zero();
    if (!canQueue(required) || isFailure(_output.reserveAdditional(required))) {
        return NetworkSendStatus::WouldBlock;
    }
    if (_framing == Http1BodyFraming::Chunked) {
        appendLiteral("0\r\n");
        appendHeaders(trailers);
        appendLiteral("\r\n");
    }
    _complete = true;
    return NetworkSendStatus::Accepted;
}

auto Http1Encoder::takeOutput(const unit::ByteLength maximumLength) -> mem::ByteBlock {
    return _output.read(maximumLength);
}

void Http1Encoder::beginRequest(const HttpRequestHead &head) {
    const auto method = head.method().text();
    const auto version = head.version().toString();
    const auto required = method.length() + head.target().length() + version.length() +
        head.headers().serializedLength() + unit::ByteLength{6U};
    if (!canQueue(required) || isFailure(_output.reserveAdditional(required))) {
        misuse("The serialized HTTP request head exceeds the output queue limit."_el);
    }
    appendString(method);
    appendLiteral(" ");
    appendString(head.target());
    appendLiteral(" ");
    appendString(version);
    appendLiteral("\r\n");
    appendHeaders(head.headers());
    appendLiteral("\r\n");
}

void Http1Encoder::beginResponse(const HttpResponseHead &head, const HttpMethod &) {
    const auto version = head.version().toString();
    const auto status = head.status().toString();
    const auto required = version.length() + status.length() + head.reasonPhrase().length() +
        head.headers().serializedLength() + unit::ByteLength{6U};
    if (!canQueue(required) || isFailure(_output.reserveAdditional(required))) {
        misuse("The serialized HTTP response head exceeds the output queue limit."_el);
    }
    appendString(version);
    appendLiteral(" ");
    appendString(status);
    appendLiteral(" ");
    appendString(head.reasonPhrase());
    appendLiteral("\r\n");
    appendHeaders(head.headers());
    appendLiteral("\r\n");
}

void Http1Encoder::selectRequestFraming(const HttpRequestHead &head) {
    selectFieldFraming(head.headers());
    if (_transferCoding != Http1TransferCoding::None) {
        if (head.version() == HttpVersion::Http10 || _transferCoding != Http1TransferCoding::Chunked) {
            misuse("Only HTTP/1.1 chunked transfer coding can be encoded."_el);
        }
        _framing = Http1BodyFraming::Chunked;
    } else if (_contentLength) {
        _framing = Http1BodyFraming::FixedLength;
        _bodyRemaining = *_contentLength;
    } else {
        _framing = Http1BodyFraming::None;
    }
}

void Http1Encoder::selectResponseFraming(const HttpResponseHead &head, const HttpMethod &requestMethod) {
    selectFieldFraming(head.headers());
    const auto methodType = requestMethod.standardType();
    const auto isHead = methodType && *methodType == HttpMethodType::Head;
    const auto isConnect = methodType && *methodType == HttpMethodType::Connect;
    if (head.status() == HttpStatus::SwitchingProtocols || (isConnect && head.status().isSuccessful())) {
        _framing = Http1BodyFraming::Opaque;
        return;
    }
    if (_transferCoding != Http1TransferCoding::None &&
        (head.status().isInformational() || head.status() == HttpStatus::NoContent)) {
        misuse("Transfer-Encoding is forbidden for this response status."_el);
    }
    if (_contentLength && (head.status().isInformational() || head.status() == HttpStatus::NoContent)) {
        misuse("Content-Length is forbidden for this response status."_el);
    }
    if (isHead || head.status().isInformational() || head.status() == HttpStatus::NoContent ||
        head.status() == HttpStatus::NotModified) {
        _framing = Http1BodyFraming::None;
    } else if (_transferCoding != Http1TransferCoding::None) {
        if (head.version() == HttpVersion::Http10 || _transferCoding != Http1TransferCoding::Chunked) {
            misuse("Only HTTP/1.1 chunked transfer coding can be encoded."_el);
        }
        _framing = Http1BodyFraming::Chunked;
    } else if (_contentLength) {
        _framing = Http1BodyFraming::FixedLength;
        _bodyRemaining = *_contentLength;
    } else {
        _framing = Http1BodyFraming::CloseDelimited;
    }
}

void Http1Encoder::selectFieldFraming(const HttpHeaders &headers) {
    try {
        const auto parser = Http1FramingParser{headers};
        if (const auto value = parser.contentLength()) {
            if (*value > _limits.maximumBodyLength().toSizeT()) {
                misuse("The declared body exceeds the encoder body limit."_el);
            }
            _contentLength = unit::ByteLength{*value};
        } else {
            _contentLength.reset();
        }
        _transferCoding = parser.transferCoding();
        if (_contentLength && _transferCoding != Http1TransferCoding::None) {
            misuse("Content-Length and Transfer-Encoding cannot be combined."_el);
        }
    } catch (const err::ParseError &) {
        misuse("The HTTP message framing fields are malformed or ambiguous."_el);
    }
}

auto Http1Encoder::canQueue(const unit::ByteLength length) const noexcept -> bool {
    const auto maximum = _limits.maximumOutputLength().toSizeT();
    return length.toSizeT() <= maximum && _output.length().toSizeT() <= maximum - length.toSizeT();
}

void Http1Encoder::appendString(const text::String &value) {
    const auto chars = text::impl::UnsafeU8StringAccess{value}.dataSpan();
    if (isFailure(_output.writeExact(mem::toConstByteSpan(chars)))) {
        std::terminate();
    }
}

void Http1Encoder::appendLiteral(const std::string_view value) {
    if (isFailure(_output.writeExact(mem::toConstByteSpan(std::span<const char>{value.data(), value.size()})))) {
        std::terminate();
    }
}

void Http1Encoder::appendHeaders(const HttpHeaders &headers) {
    const auto fields = headers.fields();
    for (const auto &field : fields) {
        appendString(field.name().text());
        appendLiteral(": ");
        appendString(field.value());
        appendLiteral("\r\n");
    }
}

void Http1Encoder::appendChunkPrefix(const unit::ByteLength length) {
    auto buffer = std::array<char, 2U * sizeof(std::size_t)>{};
    const auto result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), length.toSizeT(), 16);
    if (isFailure(_output.writeExact(mem::toConstByteSpan(std::span<const char>{buffer.data(), result.ptr})))) {
        std::terminate();
    }
    appendLiteral("\r\n");
}

auto Http1Encoder::chunkWireLength(const unit::ByteLength length) noexcept -> unit::ByteLength {
    auto value = length.toSizeT();
    auto digits = std::size_t{1U};
    while (value >= 16U) {
        value /= 16U;
        ++digits;
    }
    return length + unit::ByteLength{digits + 4U};
}

auto Http1Encoder::trailerWireLength(const HttpHeaders &trailers) const -> unit::ByteLength {
    validateFields(trailers, _limits.trailerLimits());
    const auto fields = trailers.fields();
    for (const auto &field : fields) {
        if (field.name().type().isKnown()) {
            misuse("Recognized HTTP fields are forbidden in trailers."_el);
        }
    }
    return trailers.serializedLength();
}

void Http1Encoder::validateFields(const HttpHeaders &headers, const HttpHeaderLimits limits) const {
    if (headers.fieldCount() > limits.maximumFieldCount() ||
        headers.serializedLength() > limits.maximumAggregateLength()) {
        misuse("An HTTP field section exceeds configured limits."_el);
    }
    const auto fields = headers.fields();
    for (const auto &field : fields) {
        if (field.name().text().length() > limits.maximumNameLength() ||
            field.value().length() > limits.maximumValueLength()) {
            misuse("An HTTP field component exceeds configured limits."_el);
        }
    }
}

void Http1Encoder::misuse(const text::String &message) {
    throw Http1ProtocolError{Http1FailureReason::InvalidState, message};
}

}
