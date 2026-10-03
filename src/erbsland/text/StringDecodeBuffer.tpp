// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Literals.hpp"

#include "impl/ThrowHelper.hpp"
#include "impl/UnsafeU8StringAccess.hpp"

#include "../mem/impl/SecureErase.hpp"
#include "../mem/impl/UnsafeByteBlockAccess.hpp"
#include "../util/LoopResult.hpp"
#include "../util/LoopStatus.hpp"

namespace erbsland::text {

inline StringDecodeBuffer::StringDecodeBuffer(
    const unit::ByteCount bufferLength,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingMode mode) :
    _buffer{bufferLength},
    _encoding{encoding},
    _effectiveEncoding{encoding.effectiveEncoding()},
    _bomMode{bomMode},
    _mode{mode} {
    using namespace literals;
    if (bufferLength < unit::ByteCount{4U}) {
        throw err::ParameterFailure{"Decode buffer must have at least four bytes."_el, "bufferLength"_el};
    }
}

inline auto StringDecodeBuffer::decodableCharacters(const unit::CpCount maximum) -> unit::CpCount {
    return decodedRange(maximum, false).characterLength;
}

inline auto StringDecodeBuffer::codePointStatus() -> CodePointStatus {
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

inline void StringDecodeBuffer::write(const mem::ConstByteSpan bytes) {
    using namespace literals;
    const auto inputLength = unit::ByteCount::fromSizeT(bytes.size());
    if (inputLength > availableSpace()) {
        throw err::ParameterFailure{"Decode buffer has not enough available space."_el, "bytes"_el};
    }
    auto remaining = inputLength;
    auto sourceIndex = unit::ByteIndex{};
    while (!remaining.isZero()) {
        auto destination = writableSpan();
        const auto count = std::min(unit::ByteCount::fromSizeT(destination.size()), remaining);
        std::memcpy(destination.data(), bytes.data() + sourceIndex.toSizeT(), count.toSizeT() * sizeof(mem::Byte));
        commitWritten(count);
        sourceIndex += count;
        remaining -= count;
    }
}

inline void StringDecodeBuffer::write(const mem::ByteBlock &bytes) {
    write(mem::impl::UnsafeByteBlockAccess{bytes}.dataView().dataSpan());
}

inline void StringDecodeBuffer::write(const std::vector<mem::Byte> &bytes) {
    write(mem::ConstByteSpan{bytes});
}

inline void StringDecodeBuffer::write(const std::vector<uint8_t> &bytes) {
    write(mem::toConstByteSpan(std::span<const uint8_t>{bytes}));
}

inline void StringDecodeBuffer::write(const std::vector<char> &bytes) {
    write(mem::toConstByteSpan(std::span<const char>{bytes}));
}

inline void StringDecodeBuffer::write(const std::string_view bytes) {
    write(mem::toConstByteSpan(std::span<const char>{bytes}));
}

inline void StringDecodeBuffer::writeStringBytes(const String &bytes) {
    write(mem::toConstByteSpan(impl::UnsafeU8StringAccess{bytes}.dataView().dataSpan()));
}

inline void StringDecodeBuffer::reset() noexcept {
    resetStorage();
    _readIndex = {};
    _byteLength = {};
    _effectiveEncoding = _encoding.effectiveEncoding();
    _finished = false;
    _bomResolved = false;
    _consumedByteLength = {};
}

inline auto StringDecodeBuffer::readChar() -> std::optional<Char> {
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
        if (_mode == EncodingMode::Strict) {
            impl::throwEncodingError("Invalid encoded character");
        }
        consume(errorLength);
        return Char::replacement();
    }
    return std::nullopt;
}

inline auto StringDecodeBuffer::writableSpan() noexcept -> mem::ByteSpan {
    if (_byteLength >= capacity()) {
        return {};
    }
    const auto index = writeIndex();
    const auto freeLength = availableSpace();
    const auto contiguousLength = index < _readIndex ? std::min(freeLength, index.absoluteDistanceTo(_readIndex))
                                                     : std::min(freeLength, capacity() - index.distanceFromZero());
    return storageWritableSpan().subspan(index.toSizeT(), contiguousLength.toSizeT());
}

inline void StringDecodeBuffer::commitWritten(const unit::ByteCount length) {
    using namespace literals;
    if (length > availableSpace()) {
        throw err::ParameterFailure{"Committed byte count exceeds available buffer space."_el, "length"_el};
    }
    _byteLength += length;
}

inline auto StringDecodeBuffer::writeIndex() const noexcept -> unit::ByteIndex {
    auto result = _readIndex + _byteLength;
    if (!result.isWithin(capacity())) {
        result -= capacity();
    }
    return result;
}

inline auto StringDecodeBuffer::byteAt(const unit::ByteIndex index) const noexcept -> mem::Byte {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    auto storageIndex = _readIndex + index.distanceFromZero();
    if (!storageIndex.isWithin(capacity())) {
        storageIndex -= capacity();
    }
    return _buffer.span()[storageIndex.toSizeT()];
}

inline auto StringDecodeBuffer::byteMatches(const mem::ConstByteSpan prefix) const noexcept -> bool {
    const auto prefixLength = unit::ByteCount::fromSizeT(prefix.size());
    if (_byteLength < prefixLength) {
        return false;
    }
    return prefixLength.forEach([&](const unit::ByteIndex index) -> util::LoopStatus {
        if (byteAt(index) != prefix[index.toSizeT()]) {
            return util::LoopStatus::Stop;
        }
        return util::LoopStatus::Continue;
    }) == util::LoopResult::Success;
}

inline auto StringDecodeBuffer::byteMatchesAvailable(const mem::ConstByteSpan prefix) const noexcept -> bool {
    const auto prefixLength = unit::ByteCount::fromSizeT(prefix.size());
    const auto count = std::min(_byteLength, prefixLength);
    if (count.isZero() || count >= prefixLength) {
        return false;
    }
    return count.forEach([&](const unit::ByteIndex index) -> util::LoopStatus {
        if (byteAt(index) != prefix[index.toSizeT()]) {
            return util::LoopStatus::Stop;
        }
        return util::LoopStatus::Continue;
    }) == util::LoopResult::Success;
}

inline void StringDecodeBuffer::consume(const unit::ByteCount length) noexcept {
    const auto consumedLength = std::min(length, _byteLength);
    _consumedByteLength += consumedLength;
    if (isSensitive()) {
        erasePrefix(consumedLength);
    }
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

inline void StringDecodeBuffer::erasePrefix(const unit::ByteCount length) noexcept {
    auto remaining = length.toSizeT();
    auto index = _readIndex.toSizeT();
    auto storage = storageWritableSpan();
    while (remaining > 0U) {
        const auto count = std::min(remaining, storage.size() - index);
        mem::impl::secureErase(std::as_writable_bytes(storage.subspan(index, count)));
        remaining -= count;
        index = 0U;
    }
}

inline void StringDecodeBuffer::resetForContinuation(const StringEncoding effectiveEncoding) noexcept {
    reset();
    _effectiveEncoding = effectiveEncoding;
    _bomResolved = true;
}

inline auto StringDecodeBuffer::isContinuationByte(const uint8_t value) noexcept -> bool {
    return (value & 0b11000000U) == 0b10000000U;
}

inline auto StringDecodeBuffer::ensureBomResolved() -> bool {
    const auto utf8Bom = StringEncoding{StringEncoding::Utf8}.bomBytes(StringBomMode::Require);
    const auto utf16LeBom = StringEncoding{StringEncoding::Utf16LittleEndian}.bomBytes(StringBomMode::Require);
    const auto utf16BeBom = StringEncoding{StringEncoding::Utf16BigEndian}.bomBytes(StringBomMode::Require);
    const auto utf32LeBom = StringEncoding{StringEncoding::Utf32LittleEndian}.bomBytes(StringBomMode::Require);
    const auto utf32BeBom = StringEncoding{StringEncoding::Utf32BigEndian}.bomBytes(StringBomMode::Require);
    if (_bomResolved) {
        return true;
    }
    if (_byteLength.isZero()) {
        if (!_finished) {
            return false;
        }
        if (_bomMode == StringBomMode::Require) {
            impl::throwEncodingError("Missing byte order mark");
        }
        _effectiveEncoding = _encoding.effectiveEncoding();
        _bomResolved = true;
        return true;
    }
    if (!_finished) {
        if (_encoding == StringEncoding::Utf8 && byteMatchesAvailable(utf8Bom)) {
            return false;
        }
        if (_encoding.isUtf16()) {
            if (byteMatchesAvailable(utf16BeBom) || byteMatchesAvailable(utf32LeBom)) {
                return false;
            }
            if (byteMatchesAvailable(utf16LeBom) && _byteLength < unit::ByteCount::fromSizeT(utf32LeBom.size())) {
                return false;
            }
        }
        if (_encoding.isUtf32() && (byteMatchesAvailable(utf32LeBom) || byteMatchesAvailable(utf32BeBom))) {
            return false;
        }
        if (_bomMode == StringBomMode::Require &&
            (byteMatchesAvailable(utf8Bom) || byteMatchesAvailable(utf16LeBom) || byteMatchesAvailable(utf16BeBom) ||
                byteMatchesAvailable(utf32LeBom) || byteMatchesAvailable(utf32BeBom))) {
            return false;
        }
    }
    auto bomEncoding = std::optional<StringEncoding>{};
    auto bomLength = unit::ByteCount{};
    if (byteMatches(utf32LeBom)) {
        bomEncoding = StringEncoding::Utf32LittleEndian;
        bomLength = StringEncoding{StringEncoding::Utf32LittleEndian}.bomLength(StringBomMode::Require);
    } else if (byteMatches(utf32BeBom)) {
        bomEncoding = StringEncoding::Utf32BigEndian;
        bomLength = StringEncoding{StringEncoding::Utf32BigEndian}.bomLength(StringBomMode::Require);
    } else if (byteMatches(utf8Bom)) {
        bomEncoding = StringEncoding::Utf8;
        bomLength = StringEncoding{StringEncoding::Utf8}.bomLength(StringBomMode::Require);
    } else if (byteMatches(utf16LeBom)) {
        bomEncoding = StringEncoding::Utf16LittleEndian;
        bomLength = StringEncoding{StringEncoding::Utf16LittleEndian}.bomLength(StringBomMode::Require);
    } else if (byteMatches(utf16BeBom)) {
        bomEncoding = StringEncoding::Utf16BigEndian;
        bomLength = StringEncoding{StringEncoding::Utf16BigEndian}.bomLength(StringBomMode::Require);
    }
    if (!bomEncoding.has_value()) {
        if (_bomMode == StringBomMode::Require) {
            impl::throwEncodingError("Missing byte order mark");
        }
        _effectiveEncoding = _encoding.effectiveEncoding();
        _bomResolved = true;
        return true;
    }
    if (_bomMode == StringBomMode::Reject) {
        impl::throwEncodingError("Unexpected byte order mark");
    }
    if (*bomEncoding == StringEncoding::Utf8 && _encoding != StringEncoding::Utf8) {
        impl::throwEncodingError("Unexpected UTF-8 byte order mark");
    }
    if (bomEncoding->isUtf16() &&
        (!_encoding.isUtf16() ||
            (_encoding == StringEncoding::Utf16LittleEndian && *bomEncoding != StringEncoding::Utf16LittleEndian) ||
            (_encoding == StringEncoding::Utf16BigEndian && *bomEncoding != StringEncoding::Utf16BigEndian))) {
        impl::throwEncodingError("Unexpected UTF-16 byte order mark");
    }
    if (bomEncoding->isUtf32() &&
        (!_encoding.isUtf32() ||
            (_encoding == StringEncoding::Utf32LittleEndian && *bomEncoding != StringEncoding::Utf32LittleEndian) ||
            (_encoding == StringEncoding::Utf32BigEndian && *bomEncoding != StringEncoding::Utf32BigEndian))) {
        impl::throwEncodingError("Unexpected UTF-32 byte order mark");
    }
    _effectiveEncoding = *bomEncoding;
    consume(bomLength);
    _bomResolved = true;
    return true;
}

inline auto StringDecodeBuffer::decodableByteLength(
    const unit::CpCount maximum, unit::CpCount *characterCount, bool *isValid) -> unit::ByteCount {
    if (characterCount != nullptr) {
        *characterCount = unit::CpCount::zero();
    }
    if (isValid != nullptr) {
        *isValid = true;
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return unit::ByteCount::zero();
    }
    auto index = unit::ByteIndex{};
    auto count = unit::CpCount::zero();
    while (index.isWithin(_byteLength) && (maximum.isInfinite() || count < maximum)) {
        const auto result = scanCodePoint(index);
        if (result.status == CodePointStatus::Complete) {
            index += result.byteLength;
            ++count;
            continue;
        }
        if (result.status == CodePointStatus::NeedMoreData && !_finished) {
            break;
        }
        const auto remainingLength = _byteLength - index.distanceFromZero();
        const auto errorLength = result.byteLength.isZero() ? remainingLength : result.byteLength;
        index += std::min(errorLength, remainingLength);
        if (isValid != nullptr) {
            *isValid = false;
        }
        if (_mode == EncodingMode::Strict) {
            break;
        }
        ++count;
    }
    if (characterCount != nullptr) {
        *characterCount = count;
    }
    return index.distanceFromZero();
}

inline auto StringDecodeBuffer::lineByteLength(
    const unit::CpCount maximum, unit::CpCount *characterCount, bool *isValid) -> unit::ByteCount {
    if (characterCount != nullptr) {
        *characterCount = unit::CpCount::zero();
    }
    if (isValid != nullptr) {
        *isValid = true;
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return unit::ByteCount::zero();
    }
    auto index = unit::ByteIndex{};
    auto count = unit::CpCount::zero();
    while (index.isWithin(_byteLength) && (maximum.isInfinite() || count < maximum)) {
        const auto result = scanCodePoint(index);
        if (result.status == CodePointStatus::Complete) {
            const auto character = decodeCharacter(index, result.byteLength);
            index += result.byteLength;
            ++count;
            if (character == U'\n') {
                break;
            }
            continue;
        }
        if (result.status == CodePointStatus::NeedMoreData && !_finished) {
            break;
        }
        const auto remainingLength = _byteLength - index.distanceFromZero();
        const auto errorLength = result.byteLength.isZero() ? remainingLength : result.byteLength;
        index += std::min(errorLength, remainingLength);
        if (isValid != nullptr) {
            *isValid = false;
        }
        if (_mode == EncodingMode::Strict) {
            break;
        }
        ++count;
    }
    if (characterCount != nullptr) {
        *characterCount = count;
    }
    return index.distanceFromZero();
}

inline auto StringDecodeBuffer::decodedRange(const unit::CpCount maximum, const bool stopAtLineEnd) -> DecodedRange {
    auto characterLength = unit::CpCount{};
    auto isValid = true;
    const auto byteLength = stopAtLineEnd ? lineByteLength(maximum, &characterLength, &isValid)
                                          : decodableByteLength(maximum, &characterLength, &isValid);
    return {byteLength, characterLength, isValid};
}

template <typename Function>
inline void StringDecodeBuffer::forEachDecodedCharacter(
    const DecodedRange range, const bool consumeDecoded, Function function) {
    if (range.byteLength.isZero()) {
        return;
    }
    auto index = unit::ByteIndex{};
    while (index.isWithin(range.byteLength)) {
        const auto result = scanCodePoint(index);
        if (result.status == CodePointStatus::Complete) {
            const auto character = decodeCharacter(index, result.byteLength);
            index += result.byteLength;
            function(character);
            continue;
        }
        const auto remainingLength = range.byteLength - index.distanceFromZero();
        const auto errorLength = result.byteLength.isZero() ? remainingLength : result.byteLength;
        index += std::min(errorLength, remainingLength);
        if (_mode == EncodingMode::Strict) {
            impl::throwEncodingError("Invalid encoded character");
        }
        function(Char::replacement());
    }
    if (consumeDecoded) {
        consume(range.byteLength);
    }
}

inline void StringDecodeBuffer::copyUtf8Range(
    const DecodedRange range, const std::span<char> destination, const bool consumeDecoded) {
    if (!range.isValid || !_effectiveEncoding.isUtf8() || destination.size() < range.byteLength.toSizeTOrThrow()) {
        std::terminate();
    }
    const auto storage = _buffer.span();
    const auto firstLength = std::min(range.byteLength.toSizeT(), storage.size() - _readIndex.toSizeT());
    std::memcpy(destination.data(), storage.data() + _readIndex.toSizeT(), firstLength);
    const auto secondLength = range.byteLength.toSizeT() - firstLength;
    if (secondLength > 0U) {
        std::memcpy(destination.data() + firstLength, storage.data(), secondLength);
    }
    if (consumeDecoded) {
        consume(range.byteLength);
    }
}

}
