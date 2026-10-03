// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

namespace erbsland::text {

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
    const unit::ByteIndex index, const unit::ByteCount byteLength) const noexcept -> Char {
    if (_effectiveEncoding.isUtf16()) {
        const auto firstUnit = readUInt16(index);
        if (!Char::isHighSurrogate(firstUnit) || byteLength < unit::ByteCount{4U}) {
            return Char{static_cast<char32_t>(firstUnit)};
        }
        const auto secondUnit = readUInt16(index + unit::ByteCount{2U});
        return Char{
            char32_t{0x10000U} + (static_cast<char32_t>(firstUnit - 0xD800U) << 10U) +
            static_cast<char32_t>(secondUnit - 0xDC00U)};
    }
    if (_effectiveEncoding.isUtf32()) {
        return Char{readUInt32(index)};
    }
    const auto firstByte = byteAt(index).toUInt8();
    if (byteLength == unit::ByteCount{1U}) {
        return Char{static_cast<char32_t>(firstByte)};
    }
    auto codePoint = char32_t{0U};
    if (byteLength == unit::ByteCount{2U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00011111U);
    } else if (byteLength == unit::ByteCount{3U}) {
        codePoint = static_cast<char32_t>(firstByte & 0b00001111U);
    } else {
        codePoint = static_cast<char32_t>(firstByte & 0b00000111U);
    }
    for (auto offset = unit::ByteCount{1U}; offset < byteLength; offset.uncheckedIncrement()) {
        codePoint =
            (codePoint << 6U) | static_cast<char32_t>(byteAt(index.uncheckedAdvanced(offset)).toUInt8() & 0b00111111U);
    }
    return Char{codePoint};
}

inline auto StringDecodeBuffer::scanUtf8(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto firstByte = byteAt(index).toUInt8();
    if (firstByte < 0x80U) {
        return {CodePointStatus::Complete, unit::ByteCount{1U}};
    }
    if (isContinuationByte(firstByte)) {
        return {CodePointStatus::Invalid, unit::ByteCount{1U}};
    }
    auto required = unit::ByteCount{};
    if ((firstByte & 0b11100000U) == 0b11000000U && firstByte >= 0b11000010U) {
        required = unit::ByteCount{2U};
    } else if ((firstByte & 0b11110000U) == 0b11100000U) {
        required = unit::ByteCount{3U};
    } else if ((firstByte & 0b11111000U) == 0b11110000U && firstByte < 0b11110101U) {
        required = unit::ByteCount{4U};
    } else {
        return {CodePointStatus::Invalid, unit::ByteCount{1U}};
    }
    const auto available = _byteLength - index.distanceFromZero();
    const auto scanLength = std::min(available, required);
    for (auto offset = unit::ByteCount{1U}; offset < scanLength; offset.uncheckedIncrement()) {
        if (!isContinuationByte(byteAt(index.uncheckedAdvanced(offset)).toUInt8())) {
            return {CodePointStatus::Invalid, unit::ByteCount{1U}};
        }
    }
    if (available >= unit::ByteCount{2U}) {
        const auto second = byteAt(index + unit::ByteCount{1U}).toUInt8();
        if ((firstByte == 0xE0U && second < 0xA0U) || (firstByte == 0xEDU && second > 0x9FU) ||
            (firstByte == 0xF0U && second < 0x90U) || (firstByte == 0xF4U && second > 0x8FU)) {
            return {CodePointStatus::Invalid, unit::ByteCount{1U}};
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
    if (available < unit::ByteCount{2U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    const auto first = readUInt16(index);
    if (Char::isLowSurrogate(first)) {
        return {CodePointStatus::Invalid, unit::ByteCount{2U}};
    }
    if (!Char::isHighSurrogate(first)) {
        return Char{static_cast<char32_t>(first)}.isValidUnicode()
            ? ScanResult{CodePointStatus::Complete, unit::ByteCount{2U}}
            : ScanResult{CodePointStatus::Invalid, unit::ByteCount{2U}};
    }
    if (available < unit::ByteCount{4U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    return Char::isLowSurrogate(readUInt16(index + unit::ByteCount{2U}))
        ? ScanResult{CodePointStatus::Complete, unit::ByteCount{4U}}
        : ScanResult{CodePointStatus::Invalid, unit::ByteCount{2U}};
}

inline auto StringDecodeBuffer::scanUtf32(const unit::ByteIndex index) const noexcept -> ScanResult {
    if (!index.isWithin(_byteLength)) {
        return {};
    }
    const auto available = _byteLength - index.distanceFromZero();
    if (available < unit::ByteCount{4U}) {
        return {CodePointStatus::NeedMoreData, available};
    }
    return Char{readUInt32(index)}.isValidUnicode() ? ScanResult{CodePointStatus::Complete, unit::ByteCount{4U}}
                                                    : ScanResult{CodePointStatus::Invalid, unit::ByteCount{4U}};
}

inline auto StringDecodeBuffer::readUInt16(const unit::ByteIndex index) const noexcept -> char16_t {
    const auto first = byteAt(index).toUInt8();
    const auto second = byteAt(index + unit::ByteCount{1U}).toUInt8();
    return _effectiveEncoding == StringEncoding::Utf16BigEndian
        ? static_cast<char16_t>((static_cast<uint16_t>(first) << 8U) | static_cast<uint16_t>(second))
        : static_cast<char16_t>(static_cast<uint16_t>(first) | (static_cast<uint16_t>(second) << 8U));
}

inline auto StringDecodeBuffer::readUInt32(const unit::ByteIndex index) const noexcept -> char32_t {
    const auto b0 = static_cast<uint32_t>(byteAt(index).toUInt8());
    const auto b1 = static_cast<uint32_t>(byteAt(index + unit::ByteCount{1U}).toUInt8());
    const auto b2 = static_cast<uint32_t>(byteAt(index + unit::ByteCount{2U}).toUInt8());
    const auto b3 = static_cast<uint32_t>(byteAt(index + unit::ByteCount{3U}).toUInt8());
    return _effectiveEncoding == StringEncoding::Utf32BigEndian
        ? static_cast<char32_t>((b0 << 24U) | (b1 << 16U) | (b2 << 8U) | b3)
        : static_cast<char32_t>(b0 | (b1 << 8U) | (b2 << 16U) | (b3 << 24U));
}

}
