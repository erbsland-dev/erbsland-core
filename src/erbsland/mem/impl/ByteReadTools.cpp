// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteReadTools.hpp"

#include "ByteIntegerAccess.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/OutOfRangeFailure.hpp"
#include "../../err/OverflowError.hpp"
#include "../../err/ParseError.hpp"
#include "../../math/IntegerMath.hpp"

#include <limits>

namespace erbsland::mem::impl {

using namespace err;
using namespace unit;
using namespace text::literals;

auto ByteReadTools::getInteger(const ByteIndex index, const ByteIntegerFormat format, const Endianness endianness) const
    -> std::optional<IntegerValue> {
    if (!hasCompleteInteger(index, format)) {
        return std::nullopt;
    }
    try {
        return getIntegerOrThrow(index, format, endianness);
    } catch (const ParseError &) {
        return std::nullopt;
    } catch (const OverflowError &) {
        return std::nullopt;
    }
}

auto ByteReadTools::hasCompleteInteger(const ByteIndex index, const ByteIntegerFormat format) const noexcept -> bool {
    const auto bytes = _data.dataSpan();
    if (!index.isValid() || index.toSizeT() >= bytes.size()) {
        return false;
    }
    const auto offset = index.toSizeT();
    if (!format.isVariableLength()) {
        return format.byteCount().toSizeT() <= bytes.size() - offset;
    }
    if (format == ByteIntegerFormat::UnsignedBase128) {
        for (auto position = offset; position < bytes.size() && position - offset < 10U; ++position) {
            if ((bytes[position].toUInt8() & 0x80U) == 0U) {
                return true;
            }
        }
        // More than ten continuation bytes must fail decoding as overflow; avoid scanning unbounded input.
        return bytes.size() - offset > 10U;
    }
    const auto first = bytes[offset].toUInt8();
    auto byteCount = std::size_t{1U};
    for (auto mask = uint8_t{0x80U}; mask != 0U && (first & mask) != 0U; mask >>= 1U) {
        ++byteCount;
    }
    return byteCount <= bytes.size() - offset;
}

auto ByteReadTools::getIntegerOrThrow(
    const ByteIndex index, const ByteIntegerFormat format, const Endianness endianness) const -> IntegerValue {
    if (!index.isValid()) {
        throw OutOfRangeError("Integer byte range out of range"_el);
    }
    const auto bytes = _data.dataSpan();
    const auto offset = index.toSizeT();
    if (!format.isVariableLength()) {
        const auto byteLength = format.byteCount();
        if (offset > bytes.size() || byteLength.toSizeT() > bytes.size() - offset) {
            throw OutOfRangeError("Integer byte range out of range"_el);
        }
        if (!format.isSigned()) {
            auto magnitude = uint64_t{};
            switch (byteLength.toSizeT()) {
            case 1U:
                magnitude = getIntegerUnchecked<uint8_t>(bytes, offset, endianness);
                break;
            case 2U:
                magnitude = getIntegerUnchecked<uint16_t>(bytes, offset, endianness);
                break;
            case 3U:
            case 5U:
            case 6U:
            case 7U:
                magnitude = 0U;
                if (endianness == Endianness::Big) {
                    for (auto byteIndex = std::size_t{}; byteIndex < byteLength.toSizeT(); ++byteIndex) {
                        magnitude = (magnitude << 8U) | bytes[offset + byteIndex].toUInt8();
                    }
                } else {
                    for (auto byteIndex = byteLength.toSizeT(); byteIndex > 0U; --byteIndex) {
                        magnitude = (magnitude << 8U) | bytes[offset + byteIndex - 1U].toUInt8();
                    }
                }
                break;
            case 4U:
                magnitude = getIntegerUnchecked<uint32_t>(bytes, offset, endianness);
                break;
            case 8U:
                magnitude = getIntegerUnchecked<uint64_t>(bytes, offset, endianness);
                break;
            default:
                throw OutOfRangeFailure("Invalid static integer byte width"_el);
            }
            return {false, magnitude, byteLength};
        }

        auto signedValue = int64_t{};
        switch (byteLength.toSizeT()) {
        case 1U:
            signedValue = getIntegerUnchecked<int8_t>(bytes, offset, endianness);
            break;
        case 2U:
            signedValue = getIntegerUnchecked<int16_t>(bytes, offset, endianness);
            break;
        case 4U:
            signedValue = getIntegerUnchecked<int32_t>(bytes, offset, endianness);
            break;
        case 8U:
            signedValue = getIntegerUnchecked<int64_t>(bytes, offset, endianness);
            break;
        default:
            throw OutOfRangeFailure("Invalid static integer byte width"_el);
        }
        return {math::isNegativeValue(signedValue), math::toUnsignedAbsolute(signedValue), byteLength};
    }

    if (offset >= bytes.size()) {
        throw OutOfRangeError("Integer byte range out of range"_el);
    }
    if (format == ByteIntegerFormat::UnsignedBase128) {
        // X.690 section 8.1.2.4.2 and 8.19 use this most-significant-group-first base-128 representation.
        if (bytes[offset].toUInt8() == 0x80U) {
            throw ParseError("Base-128 integer is not minimally encoded"_el);
        }
        auto value = uint64_t{};
        auto byteCount = std::size_t{};
        for (auto rawIndex = offset; rawIndex < bytes.size(); ++rawIndex) {
            const auto byte = bytes[rawIndex].toUInt8();
            if (value > (std::numeric_limits<uint64_t>::max() >> 7U)) {
                throw OverflowError("Base-128 integer exceeds 64 bits"_el);
            }
            value = (value << 7U) | static_cast<uint64_t>(byte & 0x7fU);
            ++byteCount;
            if ((byte & 0x80U) == 0U) {
                return {false, value, ByteCount::fromSizeT(byteCount)};
            }
        }
        throw OutOfRangeError("Integer byte range out of range"_el);
    }
    const auto first = bytes[offset].toUInt8();
    auto byteCount = std::size_t{1U};
    if (first == 0xffU) {
        byteCount = 9U;
    } else {
        for (auto mask = uint8_t{0x80U}; (first & mask) != 0U; mask >>= 1U) {
            ++byteCount;
        }
    }
    if (byteCount > bytes.size() - offset) {
        throw OutOfRangeError("Integer byte range out of range"_el);
    }
    auto rawValue = uint64_t{};
    if (byteCount != 9U) {
        const auto firstByteMask = (uint64_t{1U} << (std::size_t{8U} - byteCount)) - 1U;
        rawValue = static_cast<uint64_t>(first) & firstByteMask;
    }
    for (auto i = std::size_t{1U}; i < byteCount; ++i) {
        rawValue = (rawValue << 8U) | bytes[offset + i].toUInt8();
    }
    const auto byteLength = ByteCount::fromSizeT(byteCount);
    if (!format.isSigned()) {
        return {false, rawValue, byteLength};
    }
    const auto isNegative = (rawValue & 1U) != 0U;
    const auto magnitude = rawValue / 2U + (isNegative ? 1U : 0U);
    return {isNegative, magnitude, byteLength};
}

}
