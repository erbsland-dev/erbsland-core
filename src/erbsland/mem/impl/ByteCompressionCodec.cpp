// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteCompressionCodec.hpp"

#include "../ByteBlockEditor.hpp"
#include "../ByteCompressionError.hpp"

#include "../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace erbsland::mem::impl {

using namespace text::literals;

constexpr auto cMinimumMatchLength = std::size_t{4U};
constexpr auto cHashTableSize = std::size_t{1U << 16U};
constexpr auto cNoPosition = std::numeric_limits<std::size_t>::max();

auto ByteCompressionCodec::readUInt32(const ConstByteSpan data, const std::size_t index) noexcept -> uint32_t {
    return data[index].toUInt32() | (data[index + 1U].toUInt32() << 8U) | (data[index + 2U].toUInt32() << 16U) |
        (data[index + 3U].toUInt32() << 24U);
}

auto ByteCompressionCodec::hashSequence(const uint32_t value) noexcept -> std::size_t {
    return static_cast<std::size_t>((value * 2654435761U) >> 16U);
}

void ByteCompressionCodec::appendExtendedLength(ByteBlockEditor &output, std::size_t length) {
    while (length >= 255U) {
        output.append(Byte{255U});
        length -= 255U;
    }
    output.append(Byte{static_cast<uint8_t>(length)});
}

void ByteCompressionCodec::appendSequence(
    ByteBlockEditor &output,
    const ConstByteSpan input,
    const std::size_t literalBegin,
    const std::size_t literalLength,
    const std::size_t matchOffset,
    const std::size_t matchLength,
    const bool hasMatch) {
    const auto tokenIndex = unit::ByteIndex::end(output.length());
    output.append(Byte{});
    const auto literalToken = static_cast<uint8_t>(std::min(literalLength, std::size_t{15U}));
    const auto matchToken =
        hasMatch ? static_cast<uint8_t>(std::min(matchLength - cMinimumMatchLength, std::size_t{15U})) : uint8_t{};
    output.setOrThrow(tokenIndex, Byte{static_cast<uint8_t>((literalToken << 4U) | matchToken)});
    if (literalLength >= 15U) {
        appendExtendedLength(output, literalLength - 15U);
    }
    if (literalLength != 0U) {
        output.append(input.subspan(literalBegin, literalLength));
    }
    if (!hasMatch) {
        return;
    }
    output.append(Byte{static_cast<uint8_t>(matchOffset & 0xffU)});
    output.append(Byte{static_cast<uint8_t>((matchOffset >> 8U) & 0xffU)});
    if (matchLength - cMinimumMatchLength >= 15U) {
        appendExtendedLength(output, matchLength - cMinimumMatchLength - 15U);
    }
}

auto ByteCompressionCodec::compressLz4(const ConstByteSpan input, const bool sensitive) -> ByteBlock {
    auto output = ByteBlockEditor{};
    output.reserve(ByteCompressionAlgorithm{}.maximumCompressedLength(unit::ByteLength::fromSizeT(input.size())));
    auto table = std::array<std::size_t, cHashTableSize>{};
    table.fill(cNoPosition);
    auto anchor = std::size_t{};
    auto position = std::size_t{};
    constexpr auto cLastLiteralLength = std::size_t{5U};
    constexpr auto cLastMatchStartDistance = std::size_t{12U};
    while (position + cLastMatchStartDistance <= input.size()) {
        const auto hash = hashSequence(readUInt32(input, position));
        const auto candidate = table[hash];
        table[hash] = position;
        const auto canMatch = candidate != cNoPosition && position > candidate && position - candidate <= 65535U &&
            candidate + cMinimumMatchLength <= input.size() &&
            readUInt32(input, candidate) == readUInt32(input, position);
        if (!canMatch) {
            ++position;
            continue;
        }
        auto matchLength = cMinimumMatchLength;
        const auto maximumMatchLength = input.size() - cLastLiteralLength - position;
        while (matchLength < maximumMatchLength && input[candidate + matchLength] == input[position + matchLength]) {
            ++matchLength;
        }
        appendSequence(output, input, anchor, position - anchor, position - candidate, matchLength, true);
        position += matchLength;
        anchor = position;
    }
    appendSequence(output, input, anchor, input.size() - anchor, 0U, 0U, false);
    if (sensitive) {
        output.markAsSensitive();
    }
    return ByteBlock{output};
}

void ByteCompressionCodec::throwMalformed(const std::string_view message) {
    throw ByteCompressionError{ByteCompressionErrorReason::MalformedData, message};
}

auto ByteCompressionCodec::readExtendedLength(const ConstByteSpan input, std::size_t &position, std::size_t length)
    -> std::size_t {
    while (true) {
        if (position >= input.size()) {
            throwMalformed("LZ4 length extension is truncated.");
        }
        const auto value = input[position++].toUInt8();
        if (length > std::numeric_limits<std::size_t>::max() - value) {
            throwMalformed("LZ4 length exceeds the supported range.");
        }
        length += value;
        if (value != 255U) {
            return length;
        }
    }
}

auto ByteCompressionCodec::decompressLz4(
    const ConstByteSpan input, const unit::ByteLength originalLength, const bool sensitive) -> ByteBlock {
    if (!originalLength.isFinite()) {
        throw ByteCompressionError{ByteCompressionErrorReason::LengthMismatch, "Original length must be finite."_el};
    }
    if (input.empty()) {
        throwMalformed("LZ4 block is empty.");
    }
    auto output = ByteBlockEditor{originalLength};
    auto inputPosition = std::size_t{};
    auto outputPosition = std::size_t{};
    const auto outputSize = originalLength.toSizeTOrThrow();
    while (inputPosition < input.size()) {
        const auto token = input[inputPosition++].toUInt8();
        auto literalLength = static_cast<std::size_t>(token >> 4U);
        if (literalLength == 15U) {
            literalLength = readExtendedLength(input, inputPosition, literalLength);
        }
        if (literalLength > input.size() - inputPosition || literalLength > outputSize - outputPosition) {
            throwMalformed("LZ4 literal sequence exceeds its input or output.");
        }
        if (literalLength != 0U) {
            output.overwrite(unit::ByteIndex::fromSizeT(outputPosition), input.subspan(inputPosition, literalLength));
            inputPosition += literalLength;
            outputPosition += literalLength;
        }
        if (inputPosition == input.size()) {
            if ((token & 0x0fU) != 0U) {
                throwMalformed("LZ4 final literal sequence has match bits.");
            }
            break;
        }
        if (input.size() - inputPosition < 2U) {
            throwMalformed("LZ4 match offset is truncated.");
        }
        const auto offset = static_cast<std::size_t>(input[inputPosition].toUInt8()) |
            (static_cast<std::size_t>(input[inputPosition + 1U].toUInt8()) << 8U);
        inputPosition += 2U;
        if (offset == 0U || offset > outputPosition) {
            throwMalformed("LZ4 match offset is invalid.");
        }
        auto matchLength = static_cast<std::size_t>(token & 0x0fU) + cMinimumMatchLength;
        if ((token & 0x0fU) == 15U) {
            matchLength = readExtendedLength(input, inputPosition, matchLength);
        }
        if (matchLength > outputSize - outputPosition) {
            throwMalformed("LZ4 match exceeds the expected output length.");
        }
        for (auto index = std::size_t{}; index < matchLength; ++index) {
            const auto value = output.getOrThrow(unit::ByteIndex::fromSizeT(outputPosition - offset + index));
            output.setOrThrow(unit::ByteIndex::fromSizeT(outputPosition + index), value);
        }
        outputPosition += matchLength;
    }
    if (outputPosition != outputSize) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::LengthMismatch, "LZ4 output does not match the expected length."_el};
    }
    if (sensitive) {
        output.markAsSensitive();
    }
    return ByteBlock{output};
}

auto ByteCompressionCodec::compress(const ConstByteSpan data, const bool sensitive) const -> ByteBlock {
    if (_algorithm != ByteCompressionAlgorithm::Lz4Block) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedAlgorithm, "The compression algorithm is unsupported."_el};
    }
    return compressLz4(data, sensitive);
}

auto ByteCompressionCodec::decompress(
    const ConstByteSpan data, const unit::ByteLength originalLength, const bool sensitive) const -> ByteBlock {
    if (_algorithm != ByteCompressionAlgorithm::Lz4Block) {
        throw ByteCompressionError{
            ByteCompressionErrorReason::UnsupportedAlgorithm, "The compression algorithm is unsupported."_el};
    }
    return decompressLz4(data, originalLength, sensitive);
}

}
