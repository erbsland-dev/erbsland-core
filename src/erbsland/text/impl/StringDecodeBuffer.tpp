// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ThrowHelper.hpp"
#include "UnsafeU8StringAccess.hpp"

#include "../../mem/impl/SecureErase.hpp"
#include "../../mem/impl/UnsafeByteBlockAccess.hpp"

namespace erbsland::text {

inline StringDecodeBuffer::StringDecodeBuffer(
    const unit::ByteLength bufferLength,
    const StringEncoding encoding,
    const StringBomMode bomMode,
    const EncodingMode mode) :
    _buffer{bufferLength},
    _encoding{encoding},
    _effectiveEncoding{encoding.effectiveEncoding()},
    _bomMode{bomMode},
    _mode{mode} {
    if (bufferLength < unit::ByteLength{4U}) {
        throw err::ParameterError{"Decode buffer must have at least four bytes.", "bufferLength"};
    }
}

inline auto StringDecodeBuffer::decodableCharacters(const unit::CpLength maximum) -> unit::CpLength {
    auto count = unit::CpLength::zero();
    static_cast<void>(decodableByteLength(maximum, &count));
    return count;
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

inline void StringDecodeBuffer::write(const mem::ByteBlock &bytes) {
    write(mem::impl::UnsafeByteBlockAccess{bytes}.data());
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

inline void StringDecodeBuffer::commitWritten(const unit::ByteLength length) {
    if (length > availableSpace()) {
        throw err::ParameterError{"Committed byte count exceeds available buffer space.", "length"};
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
    const auto prefixLength = unit::ByteLength::fromSizeT(prefix.size());
    if (_byteLength < prefixLength) {
        return false;
    }
    for (auto index = unit::ByteIndex{}; index.isWithin(prefixLength); ++index) {
        if (byteAt(index) != prefix[index.toSizeT()]) {
            return false;
        }
    }
    return true;
}

inline auto StringDecodeBuffer::byteMatchesAvailable(const mem::ConstByteSpan prefix) const noexcept -> bool {
    const auto prefixLength = unit::ByteLength::fromSizeT(prefix.size());
    const auto count = std::min(_byteLength, prefixLength);
    for (auto index = unit::ByteIndex{}; index.isWithin(count); ++index) {
        if (byteAt(index) != prefix[index.toSizeT()]) {
            return false;
        }
    }
    return !count.isZero() && count < prefixLength;
}

inline void StringDecodeBuffer::consume(const unit::ByteLength length) noexcept {
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

inline void StringDecodeBuffer::erasePrefix(const unit::ByteLength length) noexcept {
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
            if (byteMatchesAvailable(utf16LeBom) && _byteLength < unit::ByteLength::fromSizeT(utf32LeBom.size())) {
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
    auto bomLength = unit::ByteLength{};
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
    const unit::CpLength maximum, unit::CpLength *characterCount, bool *isValid) -> unit::ByteLength {
    if (characterCount != nullptr) {
        *characterCount = unit::CpLength::zero();
    }
    if (isValid != nullptr) {
        *isValid = true;
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return unit::ByteLength::zero();
    }
    auto index = unit::ByteIndex{};
    auto count = unit::CpLength::zero();
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
    const unit::CpLength maximum, unit::CpLength *characterCount, bool *isValid) -> unit::ByteLength {
    if (characterCount != nullptr) {
        *characterCount = unit::CpLength::zero();
    }
    if (isValid != nullptr) {
        *isValid = true;
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return unit::ByteLength::zero();
    }
    auto index = unit::ByteIndex{};
    auto count = unit::CpLength::zero();
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

inline auto StringDecodeBuffer::decodedRange(const unit::CpLength maximum, const bool stopAtLineEnd) -> DecodedRange {
    auto characterLength = unit::CpLength{};
    auto isValid = true;
    const auto byteLength = stopAtLineEnd ? lineByteLength(maximum, &characterLength, &isValid)
                                          : decodableByteLength(maximum, &characterLength, &isValid);
    return {byteLength, characterLength, isValid};
}

template <typename Function>
inline auto StringDecodeBuffer::forEachDecodedCharacter(
    const DecodedRange range, const bool consumeDecoded, Function function) -> unit::CpLength {
    if (range.byteLength.isZero()) {
        return unit::CpLength::zero();
    }
    auto index = unit::ByteIndex{};
    auto count = unit::CpLength::zero();
    while (index.isWithin(range.byteLength)) {
        const auto result = scanCodePoint(index);
        if (result.status == CodePointStatus::Complete) {
            const auto character = decodeCharacter(index, result.byteLength);
            index += result.byteLength;
            function(character);
            ++count;
            continue;
        }
        const auto remainingLength = range.byteLength - index.distanceFromZero();
        const auto errorLength = result.byteLength.isZero() ? remainingLength : result.byteLength;
        index += std::min(errorLength, remainingLength);
        if (_mode == EncodingMode::Strict) {
            impl::throwEncodingError("Invalid encoded character");
        }
        function(Char::replacement());
        ++count;
    }
    if (consumeDecoded) {
        consume(range.byteLength);
    }
    return count;
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

inline auto StringDecodeBuffer::scanCodePoint(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (_effectiveEncoding.isUtf16()) {
        return scanUtf16(index);
    }
    if (_effectiveEncoding.isUtf32()) {
        return scanUtf32(index);
    }
    return scanUtf8(index);
}

inline auto StringDecodeBuffer::decodeCharacter(
    const unit::ByteIndex index, const unit::ByteLength byteLength) const noexcept -> Char {
    if (_effectiveEncoding.isUtf16()) {
        const auto firstUnit = readUInt16(index);
        if (!Char::isHighSurrogate(firstUnit) || byteLength < unit::ByteLength{4U}) {
            return Char{static_cast<char32_t>(firstUnit)};
        }
        const auto secondUnit = readUInt16(index + unit::ByteLength{2U});
        return Char{
            char32_t{0x10000U} + (static_cast<char32_t>(firstUnit - 0xD800U) << 10U) +
            static_cast<char32_t>(secondUnit - 0xDC00U)};
    }
    if (_effectiveEncoding.isUtf32()) {
        return Char{readUInt32(index)};
    }
    const auto firstByte = byteAt(index).toUInt8();
    if (byteLength == unit::ByteLength{1U}) {
        return Char{static_cast<char32_t>(firstByte)};
    }
    auto codePoint = char32_t{0U};
    if (byteLength == unit::ByteLength{2U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00011111U);
    } else if (byteLength == unit::ByteLength{3U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00001111U);
    } else {
        codePoint = static_cast<char32_t>(firstByte & 0b00000111U);
    }
    for (auto offset = unit::ByteLength{1U}; offset < byteLength; ++offset) {
        codePoint = (codePoint << 6U) | static_cast<char32_t>(byteAt(index + offset).toUInt8() & 0b00111111U);
    }
    return Char{codePoint};
}

inline auto StringDecodeBuffer::scanUtf8(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto firstByte = byteAt(index).toUInt8();
    if (firstByte < 0x80U) {
        return {CodePointStatus::Complete, unit::ByteLength{1U}};
    }
    if (isContinuationByte(firstByte)) {
        return {CodePointStatus::Invalid, unit::ByteLength{1U}};
    }
    auto required = unit::ByteLength{};
    if ((firstByte & 0b11100000U) == 0b11000000U && firstByte >= 0b11000010U) {
        required = unit::ByteLength{2U};
    } else if ((firstByte & 0b11110000U) == 0b11100000U) {
        required = unit::ByteLength{3U};
    } else if ((firstByte & 0b11111000U) == 0b11110000U && firstByte < 0b11110101U) {
        required = unit::ByteLength{4U};
    } else {
        return {CodePointStatus::Invalid, unit::ByteLength{1U}};
    }
    const auto available = _byteLength - index.distanceFromZero();
    for (auto offset = unit::ByteLength{1U}; offset < std::min(available, required); ++offset) {
        if (!isContinuationByte(byteAt(index + offset).toUInt8())) {
            return {CodePointStatus::Invalid, unit::ByteLength{1U}};
        }
    }
    if (available >= unit::ByteLength{2U}) {
        const auto second = byteAt(index + unit::ByteLength{1U}).toUInt8();
        if ((firstByte == 0xE0U && second < 0xA0U) || (firstByte == 0xEDU && second > 0x9FU) ||
            (firstByte == 0xF0U && second < 0x90U) || (firstByte == 0xF4U && second > 0x8FU)) {
            return {CodePointStatus::Invalid, unit::ByteLength{1U}};
        }
    }
    if (available < required) {
        return {CodePointStatus::NeedMoreData, available};
    }
    return decodeCharacter(index, required).isValidUnicode() ? ScanResult{CodePointStatus::Complete, required}
                                                             : ScanResult{CodePointStatus::Invalid, required};
}

inline auto StringDecodeBuffer::scanUtf16(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto available = _byteLength - index.distanceFromZero();
    if (available < unit::ByteLength{2U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    const auto first = readUInt16(index);
    if (Char::isLowSurrogate(first)) {
        return {CodePointStatus::Invalid, unit::ByteLength{2U}};
    }
    if (!Char::isHighSurrogate(first)) {
        return Char{static_cast<char32_t>(first)}.isValidUnicode()
            ? ScanResult{CodePointStatus::Complete, unit::ByteLength{2U}}
            : ScanResult{CodePointStatus::Invalid, unit::ByteLength{2U}};
    }
    if (available < unit::ByteLength{4U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    return Char::isLowSurrogate(readUInt16(index + unit::ByteLength{2U}))
        ? ScanResult{CodePointStatus::Complete, unit::ByteLength{4U}}
        : ScanResult{CodePointStatus::Invalid, unit::ByteLength{2U}};
}

inline auto StringDecodeBuffer::scanUtf32(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto available = _byteLength - index.distanceFromZero();
    if (available < unit::ByteLength{4U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    return Char{readUInt32(index)}.isValidUnicode() ? ScanResult{CodePointStatus::Complete, unit::ByteLength{4U}}
                                                    : ScanResult{CodePointStatus::Invalid, unit::ByteLength{4U}};
}

inline auto StringDecodeBuffer::readUInt16(const unit::ByteIndex index) const noexcept -> char16_t {
    const auto first = byteAt(index).toUInt8();
    const auto second = byteAt(index + unit::ByteLength{1U}).toUInt8();
    return _effectiveEncoding == StringEncoding::Utf16BigEndian
        ? static_cast<char16_t>((static_cast<uint16_t>(first) << 8U) | static_cast<uint16_t>(second))
        : static_cast<char16_t>(static_cast<uint16_t>(first) | (static_cast<uint16_t>(second) << 8U));
}

inline auto StringDecodeBuffer::readUInt32(const unit::ByteIndex index) const noexcept -> char32_t {
    const auto b0 = static_cast<uint32_t>(byteAt(index).toUInt8());
    const auto b1 = static_cast<uint32_t>(byteAt(index + unit::ByteLength{1U}).toUInt8());
    const auto b2 = static_cast<uint32_t>(byteAt(index + unit::ByteLength{2U}).toUInt8());
    const auto b3 = static_cast<uint32_t>(byteAt(index + unit::ByteLength{3U}).toUInt8());
    return _effectiveEncoding == StringEncoding::Utf32BigEndian
        ? static_cast<char32_t>((b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3)
        : static_cast<char32_t>(b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U));
}

}
