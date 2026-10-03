// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDecodeBuffer.hpp"

#include "impl/UnsafeU16StringBuffer.hpp"
#include "impl/UnsafeU8StringBuffer.hpp"
#include "u16/impl/U16Writer.hpp"
#include "u32/impl/U32Writer.hpp"
#include "u8/impl/U8Writer.hpp"

#include "../mem/impl/UnsafeByteBufferAccess.hpp"

#include <utility>

namespace erbsland::text {

using unit::ByteCount;
using unit::CpCount;

auto StringDecodeBuffer::peekAnyString(const CpCount maximum) -> AnyString {
    if (_effectiveEncoding.isUtf16()) {
        return peekU16String(maximum);
    }
    if (_effectiveEncoding.isUtf32()) {
        return peekU32String(maximum);
    }
    return peekU8String(maximum);
}

auto StringDecodeBuffer::peekString(const CpCount maximum) -> String {
    return peekU8String(maximum);
}

auto StringDecodeBuffer::peekU8String(const CpCount maximum) -> U8String {
    return decodeToU8(maximum, false, false).text;
}

auto StringDecodeBuffer::peekU16String(const CpCount maximum) -> U16String {
    return decodeToU16(maximum, false);
}

auto StringDecodeBuffer::peekU32String(const CpCount maximum) -> U32String {
    return decodeToU32(maximum, false);
}

void StringDecodeBuffer::setSensitive(const bool sensitive) noexcept {
    if (sensitive == isSensitive()) {
        return;
    }
    const auto bufferLength = _buffer.length();
    if (!sensitive) {
        reset();
    }
    _buffer.setSensitive(sensitive);
    if (_buffer.length().isZero() && !bufferLength.isZero()) {
        _buffer.resize(bufferLength);
    }
}

auto StringDecodeBuffer::takeAnyString(const CpCount maximum) -> AnyString {
    if (_effectiveEncoding.isUtf16()) {
        return takeU16String(maximum);
    }
    if (_effectiveEncoding.isUtf32()) {
        return takeU32String(maximum);
    }
    return takeU8String(maximum);
}

auto StringDecodeBuffer::takeString(const CpCount maximum) -> String {
    return takeU8String(maximum);
}

auto StringDecodeBuffer::takeStringLine(const CpCount maximum) -> String {
    return decodeToU8(maximum, true, true).text;
}

auto StringDecodeBuffer::takeU8String(const CpCount maximum) -> U8String {
    return decodeToU8(maximum, false, true).text;
}

auto StringDecodeBuffer::takeU16String(const CpCount maximum) -> U16String {
    return decodeToU16(maximum, true);
}

auto StringDecodeBuffer::takeU32String(const CpCount maximum) -> U32String {
    return decodeToU32(maximum, true);
}

auto StringDecodeBuffer::storageWritableSpan() noexcept -> mem::ByteSpan {
    return mem::impl::UnsafeByteBufferAccess{_buffer}.writableData();
}

auto StringDecodeBuffer::takeStringWithLength(const CpCount maximum, const bool stopAtLineEnd)
    -> std::pair<String, CpCount> {
    auto decoded = decodeToU8(maximum, stopAtLineEnd, true);
    return {std::move(decoded.text), decoded.characterLength};
}

auto StringDecodeBuffer::decodeToU8(const CpCount maximum, const bool stopAtLineEnd, const bool consumeDecoded)
    -> DecodedU8String {
    const auto range = decodedRange(maximum, stopAtLineEnd);
    if (effectiveEncoding().isUtf8() && range.isValid) {
        auto buffer = impl::UnsafeU8StringBuffer{range.byteLength, isSensitive()};
        copyUtf8Range(range, std::span<char>{buffer.data(), range.byteLength.toSizeT()}, consumeDecoded);
        return {U8String{buffer.take(range.byteLength)}, range.characterLength};
    }
    auto buffer =
        impl::UnsafeU8StringBuffer{ByteCount::fromSizeT(range.characterLength.toSizeTOrThrow()) * 4U, isSensitive()};
    auto writer = impl::U8Writer{std::span<char>{buffer.data(), buffer.capacity().toSizeT()}};
    forEachDecodedCharacter(range, consumeDecoded, [&](const Char character) -> void { writer.write(character); });
    return {U8String{buffer.take(ByteCount::fromSizeT(writer.position()))}, range.characterLength};
}

auto StringDecodeBuffer::decodeToU16(const CpCount maximum, const bool consumeDecoded) -> U16String {
    const auto range = decodedRange(maximum, false);
    auto buffer = impl::UnsafeU16StringBuffer{unit::U16Count::fromSizeT(range.characterLength.toSizeTOrThrow()) * 2U};
    auto writer = impl::U16Writer{std::span<char16_t>{buffer.data(), buffer.capacity().toSizeT()}};
    forEachDecodedCharacter(range, consumeDecoded, [&](const Char character) -> void { writer.write(character); });
    return U16String{buffer.take(writer.position())};
}

auto StringDecodeBuffer::decodeToU32(const CpCount maximum, const bool consumeDecoded) -> U32String {
    const auto range = decodedRange(maximum, false);
    auto result = U32StringEditor{};
    result.reserve(range.characterLength);
    forEachDecodedCharacter(range, consumeDecoded, [&](const Char character) -> void { result.append(character); });
    return result;
}

}
