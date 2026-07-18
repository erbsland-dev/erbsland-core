// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringDecodeBuffer.hpp"

#include "StringDecoder.hpp"

#include "impl/ThrowHelper.hpp"
#include "impl/UnsafeU8StringAccess.hpp"
#include "u16/impl/U16Encoding.hpp"
#include "u32/impl/U32Encoding.hpp"
#include "u8/impl/U8Encoding.hpp"

#include "../err/ParameterError.hpp"
#include "../mem/ByteBlock.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>

namespace erbsland::text {

using unit::ByteIndex;
using unit::ByteLength;
using unit::CpLength;

auto StringDecodeBuffer::isContinuationByte(const uint8_t value) noexcept -> bool {
    return (value & 0b11000000U) == 0b10000000U;
}

auto StringDecodeBuffer::ensureBomResolved() -> bool {
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
            text::impl::throwEncodingError("Missing byte order mark");
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
            if (byteMatchesAvailable(utf16LeBom) && _byteLength < ByteLength::fromSizeT(utf32LeBom.size())) {
                return false;
            }
        }
        if (_encoding.isUtf32() && (byteMatchesAvailable(utf32LeBom) || byteMatchesAvailable(utf32BeBom))) {
            return false;
        }
        if (_bomMode == StringBomMode::Require) {
            const auto couldBeRequiredBom = byteMatchesAvailable(utf8Bom) || byteMatchesAvailable(utf16LeBom) ||
                byteMatchesAvailable(utf16BeBom) || byteMatchesAvailable(utf32LeBom) ||
                byteMatchesAvailable(utf32BeBom);
            if (couldBeRequiredBom) {
                return false;
            }
        }
    }

    auto bomEncoding = std::optional<StringEncoding>{};
    auto bomLength = ByteLength{};
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
            text::impl::throwEncodingError("Missing byte order mark");
        }
        _effectiveEncoding = _encoding.effectiveEncoding();
        _bomResolved = true;
        return true;
    }
    if (_bomMode == StringBomMode::Reject) {
        text::impl::throwEncodingError("Unexpected byte order mark");
    }
    if (*bomEncoding == StringEncoding::Utf8 && _encoding != StringEncoding::Utf8) {
        text::impl::throwEncodingError("Unexpected UTF-8 byte order mark");
    }
    if (bomEncoding->isUtf16() &&
        (!_encoding.isUtf16() ||
            (_encoding == StringEncoding::Utf16LittleEndian && *bomEncoding != StringEncoding::Utf16LittleEndian) ||
            (_encoding == StringEncoding::Utf16BigEndian && *bomEncoding != StringEncoding::Utf16BigEndian))) {
        text::impl::throwEncodingError("Unexpected UTF-16 byte order mark");
    }
    if (bomEncoding->isUtf32() &&
        (!_encoding.isUtf32() ||
            (_encoding == StringEncoding::Utf32LittleEndian && *bomEncoding != StringEncoding::Utf32LittleEndian) ||
            (_encoding == StringEncoding::Utf32BigEndian && *bomEncoding != StringEncoding::Utf32BigEndian))) {
        text::impl::throwEncodingError("Unexpected UTF-32 byte order mark");
    }

    _effectiveEncoding = *bomEncoding;
    consume(bomLength);
    _bomResolved = true;
    return true;
}

auto StringDecodeBuffer::decodableByteLength(const CpLength maximum, CpLength *characterCount) -> ByteLength {
    if (characterCount != nullptr) {
        *characterCount = CpLength::zero();
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return ByteLength::zero();
    }

    auto index = ByteIndex{};
    auto count = CpLength::zero();
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
        if (_errorMode == EncodingErrorMode::Replace) {
            ++count;
        } else if (_errorMode == EncodingErrorMode::Throw) {
            break;
        }
    }
    if (characterCount != nullptr) {
        *characterCount = count;
    }
    return index.distanceFromZero();
}

auto StringDecodeBuffer::lineByteLength(const CpLength maximum, CpLength *characterCount) -> ByteLength {
    if (characterCount != nullptr) {
        *characterCount = CpLength::zero();
    }
    if (maximum.isZero() || !ensureBomResolved() || _byteLength.isZero()) {
        return ByteLength::zero();
    }

    auto index = ByteIndex{};
    auto count = CpLength::zero();
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
        if (_errorMode == EncodingErrorMode::Replace) {
            ++count;
        } else if (_errorMode == EncodingErrorMode::Throw) {
            break;
        }
    }
    if (characterCount != nullptr) {
        *characterCount = count;
    }
    return index.distanceFromZero();
}

auto StringDecodeBuffer::scanCodePoint(const ByteIndex index) const noexcept -> ScanResult {
    if (_effectiveEncoding.isUtf16()) {
        return scanUtf16(index);
    }
    if (_effectiveEncoding.isUtf32()) {
        return scanUtf32(index);
    }
    return scanUtf8(index);
}

auto StringDecodeBuffer::decodeCharacter(const ByteIndex index, const ByteLength byteLength) const noexcept -> Char {
    if (_effectiveEncoding.isUtf16()) {
        const auto firstUnit = readUInt16(index);
        if (!Char::isHighSurrogate(firstUnit) || byteLength < ByteLength{4U}) {
            return Char{static_cast<char32_t>(firstUnit)};
        }
        const auto secondUnit = readUInt16(index + ByteLength{2U});
        const auto codePoint = char32_t{0x10000U} + (static_cast<char32_t>(firstUnit - 0xD800U) << 10U) +
            static_cast<char32_t>(secondUnit - 0xDC00U);
        return Char{codePoint};
    }
    if (_effectiveEncoding.isUtf32()) {
        return Char{readUInt32(index)};
    }

    const auto firstByte = byteAt(index).toUInt8();
    if (byteLength == ByteLength{1U}) {
        return Char{static_cast<char32_t>(firstByte)};
    }
    auto codePoint = char32_t{0U};
    if (byteLength == ByteLength{2U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00011111U);
    } else if (byteLength == ByteLength{3U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00001111U);
    } else {
        codePoint = static_cast<char32_t>(firstByte & 0b00000111U);
    }
    for (auto offset = ByteLength{1U}; offset < byteLength; ++offset) {
        codePoint <<= 6U;
        codePoint |= static_cast<char32_t>(byteAt(index + offset).toUInt8() & 0b00111111U);
    }
    return Char{codePoint};
}

auto StringDecodeBuffer::scanUtf8(const ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto firstByte = byteAt(index).toUInt8();
    if (firstByte < 0x80U) {
        return ScanResult{CodePointStatus::Complete, ByteLength{1U}};
    }
    if (isContinuationByte(firstByte)) {
        return ScanResult{CodePointStatus::Invalid, ByteLength{1U}};
    }

    auto requiredByteCount = ByteLength{};
    if ((firstByte & 0b11100000U) == 0b11000000U && firstByte >= 0b11000010U) {
        requiredByteCount = ByteLength{2U};
    } else if ((firstByte & 0b11110000U) == 0b11100000U) {
        requiredByteCount = ByteLength{3U};
    } else if ((firstByte & 0b11111000U) == 0b11110000U && firstByte < 0b11110101U) {
        requiredByteCount = ByteLength{4U};
    } else {
        return ScanResult{CodePointStatus::Invalid, ByteLength{1U}};
    }
    const auto availableByteCount = _byteLength - index.distanceFromZero();
    const auto availableContinuationCount = std::min(availableByteCount, requiredByteCount);
    for (auto offset = ByteLength{1U}; offset < availableContinuationCount; ++offset) {
        if (!isContinuationByte(byteAt(index + offset).toUInt8())) {
            return ScanResult{CodePointStatus::Invalid, ByteLength{1U}};
        }
    }
    if (availableByteCount >= ByteLength{2U}) {
        const auto secondByte = byteAt(index + ByteLength{1U}).toUInt8();
        if ((firstByte == 0xE0U && secondByte < 0xA0U) || (firstByte == 0xEDU && secondByte > 0x9FU) ||
            (firstByte == 0xF0U && secondByte < 0x90U) || (firstByte == 0xF4U && secondByte > 0x8FU)) {
            return ScanResult{CodePointStatus::Invalid, ByteLength{1U}};
        }
    }
    if (availableByteCount < requiredByteCount) {
        return ScanResult{CodePointStatus::NeedMoreData, availableByteCount};
    }
    return ScanResult{CodePointStatus::Complete, requiredByteCount};
}

auto StringDecodeBuffer::scanUtf16(const ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto availableByteCount = _byteLength - index.distanceFromZero();
    if (availableByteCount < ByteLength{2U}) {
        return ScanResult{CodePointStatus::NeedMoreData, availableByteCount};
    }
    const auto firstUnit = readUInt16(index);
    if (Char::isLowSurrogate(firstUnit)) {
        return ScanResult{CodePointStatus::Invalid, ByteLength{2U}};
    }
    if (!Char::isHighSurrogate(firstUnit)) {
        return ScanResult{CodePointStatus::Complete, ByteLength{2U}};
    }
    if (availableByteCount < ByteLength{4U}) {
        return ScanResult{CodePointStatus::NeedMoreData, availableByteCount};
    }
    if (!Char::isLowSurrogate(readUInt16(index + ByteLength{2U}))) {
        return ScanResult{CodePointStatus::Invalid, ByteLength{2U}};
    }
    return ScanResult{CodePointStatus::Complete, ByteLength{4U}};
}

auto StringDecodeBuffer::scanUtf32(const ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto availableByteCount = _byteLength - index.distanceFromZero();
    if (availableByteCount < ByteLength{4U}) {
        return ScanResult{CodePointStatus::NeedMoreData, availableByteCount};
    }
    return Char{readUInt32(index)}.isValidUnicode() ? ScanResult{CodePointStatus::Complete, ByteLength{4U}}
                                                    : ScanResult{CodePointStatus::Invalid, ByteLength{4U}};
}

auto StringDecodeBuffer::readUInt16(const ByteIndex index) const noexcept -> char16_t {
    const auto first = byteAt(index).toUInt8();
    const auto second = byteAt(index + ByteLength{1U}).toUInt8();
    if (_effectiveEncoding == StringEncoding::Utf16BigEndian) {
        return static_cast<char16_t>((static_cast<uint16_t>(first) << 8U) | static_cast<uint16_t>(second));
    }
    return static_cast<char16_t>(static_cast<uint16_t>(first) | (static_cast<uint16_t>(second) << 8U));
}

auto StringDecodeBuffer::readUInt32(const ByteIndex index) const noexcept -> char32_t {
    const auto b0 = static_cast<uint32_t>(byteAt(index).toUInt8());
    const auto b1 = static_cast<uint32_t>(byteAt(index + ByteLength{1U}).toUInt8());
    const auto b2 = static_cast<uint32_t>(byteAt(index + ByteLength{2U}).toUInt8());
    const auto b3 = static_cast<uint32_t>(byteAt(index + ByteLength{3U}).toUInt8());
    if (_effectiveEncoding == StringEncoding::Utf32BigEndian) {
        return static_cast<char32_t>((b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3);
    }
    return static_cast<char32_t>(b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U));
}

}
