// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDecodeBuffer.hpp"

#include "StringDecoder.hpp"

#include "impl/ThrowHelper.hpp"
#include "impl/UnsafeU8StringViewAccess.hpp"
#include "u16/impl/U16Encoding.hpp"
#include "u32/impl/U32Encoding.hpp"
#include "u8/impl/U8Encoding.hpp"

#include "../err/ParameterError.hpp"
#include "../mem/ByteBlock.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <optional>

namespace erbsland::text {

StringDecodeBuffer::StringDecodeBuffer(
    const unit::ByteLength bufferLength,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingErrorMode errorMode) :
    _buffer(bufferLength.toSizeTOrThrow()),
    _encoding{encoding},
    _effectiveEncoding{defaultEffectiveEncoding(encoding)},
    _bomMode{bomMode},
    _errorMode{errorMode} {
    if (bufferLength < unit::ByteLength{4U}) {
        throw err::ParameterError{"Decode buffer must have at least four bytes.", "bufferLength"};
    }
}

auto StringDecodeBuffer::capacity() const noexcept -> unit::ByteLength {
    return unit::ByteLength::fromSizeT(_buffer.size());
}

auto StringDecodeBuffer::availableSpace() const noexcept -> unit::ByteLength {
    return capacity() - _byteLength;
}

auto StringDecodeBuffer::byteLength() const noexcept -> unit::ByteLength {
    return _byteLength;
}

auto StringDecodeBuffer::decodableCharacters(const unit::CpLength maximum) -> unit::CpLength {
    auto count = unit::CpLength::zero();
    static_cast<void>(decodableByteLength(maximum, &count));
    return count;
}

auto StringDecodeBuffer::codePointStatus() -> CodePointStatus {
    if (!ensureBomResolved()) {
        return CodePointStatus::NeedMoreData;
    }
    if (_byteLength.isZero()) {
        return CodePointStatus::Complete;
    }
    auto index = unit::ByteIndex{};
    while (index.isWithin(_byteLength)) {
        const auto result = scanCodePoint(index);
        if (result.status != CodePointStatus::Complete) {
            return result.status;
        }
        index += result.byteLength;
    }
    return CodePointStatus::Complete;
}

void StringDecodeBuffer::write(const std::span<const mem::Byte> bytes) {
    const auto inputLength = unit::ByteLength::fromSizeT(bytes.size());
    if (inputLength > availableSpace()) {
        throw err::ParameterError{"Decode buffer has not enough available space.", "bytes"};
    }
    auto remaining = inputLength;
    auto sourceIndex = unit::ByteIndex{};
    while (!remaining.isZero()) {
        auto destination = writableSpan();
        const auto count = std::min(unit::ByteLength::fromSizeT(destination.size()), remaining);
        std::memcpy(destination.data(), bytes.data() + sourceIndex.toSizeT(), count.toSizeT() * sizeof(mem::Byte));
        commitWritten(count);
        sourceIndex += count;
        remaining -= count;
    }
}

void StringDecodeBuffer::write(const mem::ByteBlockView &bytes) {
    write(bytes.bytes());
}

void StringDecodeBuffer::write(const std::vector<mem::Byte> &bytes) {
    write(std::span<const mem::Byte>{bytes});
}

void StringDecodeBuffer::write(const std::vector<uint8_t> &bytes) {
    if (unit::ByteLength::fromSizeT(bytes.size()) > availableSpace()) {
        throw err::ParameterError{"Decode buffer has not enough available space.", "bytes"};
    }
    for (const auto byte : bytes) {
        appendByte(mem::Byte{byte});
    }
}

void StringDecodeBuffer::write(const std::vector<char> &bytes) {
    write(std::string_view{bytes.data(), bytes.size()});
}

void StringDecodeBuffer::write(const std::string_view bytes) {
    if (unit::ByteLength::fromSizeT(bytes.size()) > availableSpace()) {
        throw err::ParameterError{"Decode buffer has not enough available space.", "bytes"};
    }
    for (const auto byte : bytes) {
        appendByte(mem::Byte{static_cast<uint8_t>(byte)});
    }
}

void StringDecodeBuffer::writeStringBytes(const StringView &bytes) {
    const auto data = impl::UnsafeU8StringViewAccess{bytes}.dataView().dataSpan();
    if (unit::ByteLength::fromSizeT(data.size()) > availableSpace()) {
        throw err::ParameterError{"Decode buffer has not enough available space.", "bytes"};
    }
    for (const auto byte : data) {
        appendByte(mem::Byte{static_cast<uint8_t>(byte)});
    }
}

void StringDecodeBuffer::reset() noexcept {
    _readIndex = {};
    _byteLength = {};
    _effectiveEncoding = defaultEffectiveEncoding(_encoding);
    _finished = false;
    _bomResolved = false;
    _consumedByteLength = {};
}

auto StringDecodeBuffer::peekAnyString(const unit::CpLength maximum) -> AnyString {
    if (isUtf16Encoding(_effectiveEncoding)) {
        return peekU16String(maximum);
    }
    if (isUtf32Encoding(_effectiveEncoding)) {
        return peekU32String(maximum);
    }
    return peekU8String(maximum);
}

auto StringDecodeBuffer::peekString(const unit::CpLength maximum) -> String {
    return peekU8String(maximum);
}

auto StringDecodeBuffer::peekU8String(const unit::CpLength maximum) -> U8String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    return StringDecoder{materialize(length)}.toU8String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
}

auto StringDecodeBuffer::peekU16String(const unit::CpLength maximum) -> U16String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    return StringDecoder{materialize(length)}.toU16String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
}

auto StringDecodeBuffer::peekU32String(const unit::CpLength maximum) -> U32String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    return StringDecoder{materialize(length)}.toU32String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
}

auto StringDecodeBuffer::takeAnyString(const unit::CpLength maximum) -> AnyString {
    if (isUtf16Encoding(_effectiveEncoding)) {
        return takeU16String(maximum);
    }
    if (isUtf32Encoding(_effectiveEncoding)) {
        return takeU32String(maximum);
    }
    return takeU8String(maximum);
}

auto StringDecodeBuffer::takeString(const unit::CpLength maximum) -> String {
    return takeU8String(maximum);
}

auto StringDecodeBuffer::readChar() -> std::optional<Char> {
    if (!ensureBomResolved()) {
        return std::nullopt;
    }
    while (!_byteLength.isZero()) {
        const auto result = scanCodePoint(unit::ByteIndex{});
        if (result.status == CodePointStatus::Complete) {
            const auto character = decodeCharacter(unit::ByteIndex{}, result.byteLength);
            consume(result.byteLength);
            return character;
        }
        if (result.status == CodePointStatus::NeedMoreData && !_finished) {
            return std::nullopt;
        }

        const auto errorLength = result.byteLength.isZero() ? _byteLength : std::min(result.byteLength, _byteLength);
        if (_errorMode == EncodingErrorMode::Throw) {
            text::impl::throwEncodingError("Invalid encoded character");
        }
        consume(errorLength);
        if (_errorMode == EncodingErrorMode::Replace) {
            return Char::replacement();
        }
    }
    return std::nullopt;
}

auto StringDecodeBuffer::takeStringLine(const unit::CpLength maximum) -> String {
    const auto length = lineByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    auto result = StringDecoder{materialize(length)}.toU8String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
    consume(length);
    return result;
}

auto StringDecodeBuffer::takeU8String(const unit::CpLength maximum) -> U8String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    auto result = StringDecoder{materialize(length)}.toU8String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
    consume(length);
    return result;
}

auto StringDecodeBuffer::takeU16String(const unit::CpLength maximum) -> U16String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    auto result = StringDecoder{materialize(length)}.toU16String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
    consume(length);
    return result;
}

auto StringDecodeBuffer::takeU32String(const unit::CpLength maximum) -> U32String {
    const auto length = decodableByteLength(maximum);
    if (length.isZero()) {
        return {};
    }
    auto result = StringDecoder{materialize(length)}.toU32String(_effectiveEncoding, StringBomMode::Reject, _errorMode);
    consume(length);
    return result;
}

auto StringDecodeBuffer::writableSpan() noexcept -> std::span<mem::Byte> {
    if (_byteLength >= capacity()) {
        return {};
    }
    const auto index = writeIndex();
    const auto freeLength = availableSpace();
    if (index < _readIndex) {
        const auto contiguousLength = std::min(freeLength, index.absoluteDistanceTo(_readIndex));
        return std::span<mem::Byte>{_buffer.data() + index.toSizeT(), contiguousLength.toSizeT()};
    }
    const auto contiguousLength = std::min(freeLength, capacity() - index.distanceFromZero());
    return std::span<mem::Byte>{_buffer.data() + index.toSizeT(), contiguousLength.toSizeT()};
}

void StringDecodeBuffer::commitWritten(const unit::ByteLength length) {
    if (length > availableSpace()) {
        throw err::ParameterError{"Committed byte count exceeds available buffer space.", "length"};
    }
    _byteLength += length;
}

void StringDecodeBuffer::appendByte(const mem::Byte byte) noexcept {
    _buffer[writeIndex().toSizeT()] = byte;
    ++_byteLength;
}

auto StringDecodeBuffer::writeIndex() const noexcept -> unit::ByteIndex {
    auto result = _readIndex + _byteLength;
    if (!result.isWithin(capacity())) {
        result -= capacity();
    }
    return result;
}

auto StringDecodeBuffer::byteAt(const unit::ByteIndex index) const noexcept -> mem::Byte {
    if (!index.isWithin(_byteLength) || _buffer.empty()) {
        return {};
    }
    auto storageIndex = _readIndex + index.distanceFromZero();
    if (!storageIndex.isWithin(capacity())) {
        storageIndex -= capacity();
    }
    return _buffer[storageIndex.toSizeT()];
}

auto StringDecodeBuffer::byteMatches(const std::span<const uint8_t> prefix) const noexcept -> bool {
    const auto prefixLength = unit::ByteLength::fromSizeT(prefix.size());
    if (_byteLength < prefixLength) {
        return false;
    }
    for (auto index = unit::ByteIndex{}; index.isWithin(prefixLength); ++index) {
        if (byteAt(index).toUInt8() != prefix[index.toSizeT()]) {
            return false;
        }
    }
    return true;
}

auto StringDecodeBuffer::byteMatchesAvailable(const std::span<const uint8_t> prefix) const noexcept -> bool {
    const auto prefixLength = unit::ByteLength::fromSizeT(prefix.size());
    const auto count = std::min(_byteLength, prefixLength);
    for (auto index = unit::ByteIndex{}; index.isWithin(count); ++index) {
        if (byteAt(index).toUInt8() != prefix[index.toSizeT()]) {
            return false;
        }
    }
    return !count.isZero() && count < prefixLength;
}

auto StringDecodeBuffer::materialize(const unit::ByteLength length) const -> mem::ByteBlock {
    auto bytes = std::vector<mem::Byte>{};
    bytes.reserve(length.toSizeT());
    for (auto index = unit::ByteIndex{}; index.isWithin(length); ++index) {
        bytes.push_back(byteAt(index));
    }
    return mem::ByteBlock{bytes};
}

void StringDecodeBuffer::consume(const unit::ByteLength length) noexcept {
    const auto consumedLength = std::min(length, _byteLength);
    _consumedByteLength += consumedLength;
    if (consumedLength == _byteLength) {
        _readIndex = {};
        _byteLength = {};
        return;
    }
    _readIndex += consumedLength;
    if (!_readIndex.isWithin(capacity())) {
        _readIndex -= capacity();
    }
    _byteLength -= consumedLength;
}

auto StringDecodeBuffer::consumedByteLength() const noexcept -> unit::ByteLength {
    return _consumedByteLength;
}

auto StringDecodeBuffer::isBomResolved() const noexcept -> bool {
    return _bomResolved;
}

void StringDecodeBuffer::resetForContinuation(const StringEncoding effectiveEncoding) noexcept {
    reset();
    _effectiveEncoding = effectiveEncoding;
    _bomResolved = true;
}

}
