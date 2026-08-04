// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteWriterTools.hpp"

#include "ByteTextTools.hpp"

#include "../ByteTextOptions.hpp"
#include "../ByteWriter.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../math/SignedMagnitude.hpp"
#include "../../text/String.hpp"
#include "../../text/StringEncoder.hpp"
#include "../../text/StringSide.hpp"

#include <limits>

namespace erbsland::mem::impl {

using namespace text;
using namespace text::literals;
using namespace unit;

void ByteWriterTools::writeText(const String &text, const ByteTextOptions &options, const bool strict) {
    _writer.writeBytes(makeTextFrame(text, options, strict));
}

void ByteWriterTools::writeIntegerOrThrow(
    const bool isNegative, const uint64_t magnitude, const ByteIntegerFormat format) {

    const auto rawValue = rawValueOrThrow(isNegative, magnitude, format);
    if (format == ByteIntegerFormat::UnsignedBase128) {
        encodeBase128Integer(rawValue);
        return;
    }
    if (format.isVariableLength()) {
        encodeVariableInteger(rawValue);
        return;
    }
    encodeStaticInteger(rawValue, format.byteCount());
}

auto ByteWriterTools::rawValueOrThrow(
    const bool isNegative, const uint64_t magnitude, const ByteIntegerFormat format) const -> uint64_t {
    if (!format.isSigned()) {
        if (isNegative) {
            throw err::OutOfRangeError("A negative integer cannot use an unsigned wire format"_el);
        }
        if (!format.isVariableLength()) {
            const auto bitCount = format.byteCount().toSizeT() * 8U;
            const auto maximum =
                bitCount == 64U ? std::numeric_limits<uint64_t>::max() : (uint64_t{1U} << bitCount) - 1U;
            if (magnitude > maximum) {
                throw err::OutOfRangeError("Integer does not fit the unsigned wire format"_el);
            }
        }
        return magnitude;
    }

    const auto value = math::SignedMagnitude<int64_t>{isNegative, magnitude};
    const auto bitCount = format.isVariableLength() ? std::size_t{64U} : format.byteCount().toSizeT() * 8U;
    const auto minimum = bitCount == 64U ? std::numeric_limits<int64_t>::min() : -(int64_t{1U} << (bitCount - 1U));
    const auto maximum = bitCount == 64U ? std::numeric_limits<int64_t>::max() : (int64_t{1U} << (bitCount - 1U)) - 1;
    if (value.wouldSaturate(minimum, maximum)) {
        throw err::OutOfRangeError("Integer does not fit the signed wire format"_el);
    }
    if (format.isVariableLength()) {
        return isNegative ? magnitude * 2U - 1U : magnitude * 2U;
    }
    return isNegative ? uint64_t{0U} - magnitude : magnitude;
}

void ByteWriterTools::encodeStaticInteger(const uint64_t value, const ByteLength byteCount) const {
    ByteArray<8> bytes;
    switch (byteCount.toSizeT()) {
    case 1U:
        bytes.setIntegerOrThrow(ByteIndex::zero(), static_cast<uint8_t>(value), _writer.endianness());
        break;
    case 2U:
        bytes.setIntegerOrThrow(ByteIndex::zero(), static_cast<uint16_t>(value), _writer.endianness());
        break;
    case 4U:
        bytes.setIntegerOrThrow(ByteIndex::zero(), static_cast<uint32_t>(value), _writer.endianness());
        break;
    case 8U:
        bytes.setIntegerOrThrow(ByteIndex::zero(), value, _writer.endianness());
        break;
    case 3U:
    case 5U:
    case 6U:
    case 7U:
        if (_writer.endianness() == Endianness::Big) {
            for (auto byteIndex = std::size_t{}; byteIndex < byteCount.toSizeT(); ++byteIndex) {
                const auto shift = (byteCount.toSizeT() - byteIndex - 1U) * 8U;
                bytes.set(ByteIndex{byteIndex}, Byte{static_cast<uint8_t>(value >> shift)});
            }
        } else {
            for (auto byteIndex = std::size_t{}; byteIndex < byteCount.toSizeT(); ++byteIndex) {
                bytes.set(ByteIndex{byteIndex}, Byte{static_cast<uint8_t>(value >> (byteIndex * 8U))});
            }
        }
        break;
    default:
        throw err::OutOfRangeError("Invalid static integer byte width"_el);
    }
    _writer.writeBytes(bytes.span().subspan(0U, byteCount.toSizeT()));
}

void ByteWriterTools::encodeVariableInteger(const uint64_t value) const {
    auto byteCount = std::size_t{1U};
    while (byteCount < 9U && value > ((uint64_t{1U} << (7U * byteCount)) - 1U)) {
        ++byteCount;
    }

    ByteArray<12> result;
    auto index = ByteIndex::zero();
    if (byteCount == 9U) {
        result.set(index++, 0xffU);
        for (auto shift = std::size_t{56U}; shift > 0U; shift -= 8U) {
            result.set(index++, static_cast<uint8_t>(value >> shift));
        }
        result.set(index++, static_cast<uint8_t>(value));
        _writer.writeBytes(result.span().subspan(0U, index.toSizeT()));
        return;
    }

    const auto firstShift = (byteCount - 1U) * 8U;
    const auto prefix = static_cast<uint8_t>(uint64_t{0xffU} << (std::size_t{9U} - byteCount));
    const auto payloadMask = static_cast<uint8_t>((uint64_t{1U} << (std::size_t{8U} - byteCount)) - 1U);
    result.set(index++, static_cast<uint8_t>(prefix | ((value >> firstShift) & payloadMask)));
    for (auto i = byteCount - 1U; i > 0U; --i) {
        result.set(index++, static_cast<uint8_t>(value >> ((i - 1U) * 8U)));
    }
    _writer.writeBytes(result.span().subspan(0U, index.toSizeT()));
}

void ByteWriterTools::encodeBase128Integer(const uint64_t value) const {
    // X.690 section 8.1.2.4.2 and 8.19: emit the most significant seven-bit group first.
    auto groupCount = std::size_t{1U};
    for (auto remaining = value >> 7U; remaining != 0U; remaining >>= 7U) {
        ++groupCount;
    }
    ByteArray<12> result;
    auto index = ByteIndex::zero();
    for (auto groupIndex = groupCount; groupIndex > 0U; --groupIndex) {
        const auto shift = (groupIndex - 1U) * 7U;
        const auto continuation = groupIndex > 1U ? uint8_t{0x80U} : uint8_t{};
        result.set(index++, static_cast<uint8_t>(continuation | ((value >> shift) & 0x7fU)));
    }
    _writer.writeBytes(result.span().subspan(0U, index.toSizeT()));
}

auto ByteWriterTools::makeTextFrame(const String &text, const ByteTextOptions &options, const bool strict)
    -> ByteBlock {
    if (options.format() == ByteTextFormat::PaddedField && !options.length().isFinite()) {
        throw err::OutOfRangeError("A padded text field requires a finite byte length"_el);
    }
    const auto textTools = ByteTextTools{options};
    const auto unitSize = textTools.unitSize();
    const auto encodedMark = textTools.encodedEndMark();
    const auto encodePrefix = [&](const std::size_t characterCount) -> ByteBlock {
        return StringEncoder{text.slice(StringSide::Front, CpLength::fromSizeT(characterCount))}.encode(
            options.encoding(), StringBomMode::Reject);
    };
    const auto buildFrame = [&](const ByteBlock &payload) -> ByteBlock {
        auto frame = ByteWriter{};
        frame.setEndianness(_writer.endianness());
        if (options.countFormat().has_value()) {
            frame.writeIntegerOrThrow<uint64_t>(payload.length().toRawValue() / unitSize, *options.countFormat());
        }
        frame.writeBytes(payload).writeBytes(encodedMark);
        return frame.toByteBlock();
    };
    const auto fits = [&](const ByteBlock &payload) -> bool {
        if (options.format() == ByteTextFormat::PaddedField) {
            return buildFrame(payload).length() <= options.length();
        }
        return !options.length().isFinite() || payload.length() <= options.length();
    };

    const auto characterCount = text.characterLength().toSizeT();
    auto payload = encodePrefix(characterCount);
    if (!fits(payload)) {
        if (strict) {
            throw err::OutOfRangeError(
                options.format() == ByteTextFormat::PaddedField
                    ? "Encoded text exceeds the padded field length"_el
                    : "Encoded text exceeds the configured maximum length"_el);
        }
        auto lower = std::size_t{0U};
        auto upper = characterCount;
        while (lower < upper) {
            const auto middle = lower + (upper - lower + 1U) / 2U;
            if (fits(encodePrefix(middle))) {
                lower = middle;
            } else {
                upper = middle - 1U;
            }
        }
        payload = encodePrefix(lower);
        if (!fits(payload)) {
            throw err::OutOfRangeError("Text framing does not fit the padded field length"_el);
        }
    }

    const auto frame = buildFrame(payload);
    if (options.format() != ByteTextFormat::PaddedField) {
        return frame;
    }
    auto padded = ByteBlockEditor{options.length(), options.padding()};
    padded.overwrite(frame.span());
    return padded;
}

}
