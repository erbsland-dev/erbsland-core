// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Lz4Codec.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../err/OutOfRangeFailure.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <limits>
#include <vector>

namespace erbsland::compression::impl {

using namespace err;
using namespace mem;
using namespace unit;
using namespace text::literals;

void Lz4Codec::compressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const {
    const auto workspace = uint64_t{2U * 65536U * sizeof(std::size_t)};
    if (ByteCount{workspace} > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    auto buffered = ByteBlockEditor{};
    while (!input.atEnd()) {
        auto block = input.block(transfer.bufferLength().toSizeTOrThrow());
        if (buffered.length().addedOrThrow(block.length()) > transfer.maximumBufferedLength()) {
            throw OutOfRangeError{"Fallback input limit exceeded."_el};
        }
        buffered.append(block.span());
    }
    if (maximumPayloadLength(buffered.length()) > transfer.maximumBufferedLength()) {
        throw OutOfRangeError{"Fallback output bound exceeds the buffer limit."_el};
    }
    const auto result = compressBlock(buffered);
    if (result.length() > transfer.maximumBufferedLength()) {
        throw OutOfRangeError{"Fallback output limit exceeded."_el};
    }
    output(result.span());
}

void Lz4Codec::decompressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const {
    [[maybe_unused]] const auto workspace = availableWorkspace(transfer, options.maximumWorkspaceLength());
    auto buffered = ByteBlockEditor{};
    while (!input.atEnd()) {
        auto block = input.block(transfer.bufferLength().toSizeTOrThrow());
        if (buffered.length().addedOrThrow(block.length()) > transfer.maximumBufferedLength()) {
            throw OutOfRangeError{"Fallback input limit exceeded."_el};
        }
        buffered.append(block.span());
    }
    auto boundedOptions = options;
    boundedOptions.setMaximumOutputLength(
        std::min(boundedOptions.maximumOutputLength(), transfer.maximumBufferedLength()));
    if (boundedOptions.expectedOutputLength() &&
        *boundedOptions.expectedOutputLength() > boundedOptions.maximumOutputLength()) {
        throw OutOfRangeError{"Fallback output limit exceeded."_el};
    }
    const auto result = decompressBlock(buffered, boundedOptions);
    if (result.length() > transfer.maximumBufferedLength()) {
        throw OutOfRangeError{"Fallback output limit exceeded."_el};
    }
    output(result.span());
}

auto Lz4Codec::readUInt32(const ConstByteSpan data, const std::size_t index) noexcept -> uint32_t {
    return data[index].toUInt32() | (data[index + 1U].toUInt32() << 8U) | (data[index + 2U].toUInt32() << 16U) |
        (data[index + 3U].toUInt32() << 24U);
}

auto Lz4Codec::hashSequence(const uint32_t value) noexcept -> std::size_t {
    return static_cast<std::size_t>((value * cHashMultiplier) >> 16U);
}

void Lz4Codec::appendExtendedLength(ByteBlockEditor &output, std::size_t length) {
    while (length >= cLengthExtensionMaximum) {
        output.append(Byte{cLengthExtensionMaximum});
        length -= cLengthExtensionMaximum;
    }
    output.append(Byte::fromCroppedUInt64(length));
}

void Lz4Codec::appendSequence(
    ByteBlockEditor &output,
    const ConstByteSpan input,
    const std::size_t literalBegin,
    const std::size_t literalLength,
    const std::size_t matchOffset,
    const std::size_t matchLength,
    const bool hasMatch) {
    const auto tokenIndex = ByteIndex::end(output.length());
    output.append(Byte{});
    const auto literalToken = static_cast<uint8_t>(std::min(literalLength, std::size_t{cLengthNibbleMaximum}));
    const auto matchToken = hasMatch
        ? static_cast<uint8_t>(std::min(matchLength - cMinimumMatchLength, std::size_t{cLengthNibbleMaximum}))
        : uint8_t{};
    output.setOrThrow(tokenIndex, Byte::fromCroppedUInt16(static_cast<uint16_t>((literalToken << 4U) | matchToken)));
    if (literalLength >= cLengthNibbleMaximum) {
        appendExtendedLength(output, literalLength - cLengthNibbleMaximum);
    }
    if (literalLength != 0U) {
        output.append(input.subspan(literalBegin, literalLength));
    }
    if (!hasMatch) {
        return;
    }
    output.append(Byte::fromCroppedUInt64(matchOffset));
    output.append(Byte::fromCroppedUInt64(matchOffset >> 8U));
    if (matchLength - cMinimumMatchLength >= cLengthNibbleMaximum) {
        appendExtendedLength(output, matchLength - cMinimumMatchLength - cLengthNibbleMaximum);
    }
}

auto Lz4Codec::compressBlock(const ByteBlock &data) const -> ByteBlock {
    const auto input = data.span();
    auto output = ByteBlockEditor{};
    const auto inputLength = ByteCount::fromSizeT(input.size());
    output.reserve(
        inputLength.addedOrThrow(ByteCount::fromSizeT(input.size() / cLengthExtensionMaximum + cSizeOverhead)));
    auto table = std::vector<std::size_t>(cHashTableSize, cNoPosition);
    auto previous = std::vector<std::size_t>(std::min(input.size(), cMaximumOffset + 1U), cNoPosition);
    const auto depth = cSearchDepths[static_cast<std::size_t>(level())];
    auto anchor = std::size_t{};
    auto position = std::size_t{};
    while (position + cLastMatchStartDistance <= input.size()) {
        const auto hash = hashSequence(readUInt32(input, position));
        auto candidate = table[hash];
        previous[position < previous.size() ? position : position & cMaximumOffset] = candidate;
        table[hash] = position;
        const auto [bestLength, bestOffset] = findMatch(input, position, candidate, previous, depth);
        if (bestLength < cMinimumMatchLength) {
            ++position;
            continue;
        }
        if (level() >= CompressionLevel::High && position + 1U + cLastMatchStartDistance <= input.size()) {
            const auto nextHash = hashSequence(readUInt32(input, position + 1U));
            const auto nextBest = findMatch(input, position + 1U, table[nextHash], previous, depth).first;
            if (nextBest > bestLength + 1U) {
                ++position;
                continue;
            }
        }
        appendSequence(output, input, anchor, position - anchor, bestOffset, bestLength, true);
        position += bestLength;
        anchor = position;
    }
    appendSequence(output, input, anchor, input.size() - anchor, 0U, 0U, false);
    return output;
}

auto Lz4Codec::findMatch(
    const ConstByteSpan input,
    const std::size_t position,
    std::size_t candidate,
    const std::span<const std::size_t> previous,
    const std::size_t depth) noexcept -> std::pair<std::size_t, std::size_t> {
    auto bestLength = std::size_t{};
    auto bestOffset = std::size_t{};
    auto attempts = std::size_t{};
    const auto maximumMatchLength = input.size() - cLastLiteralLength - position;
    while (candidate != cNoPosition && attempts++ < depth) {
        if (position <= candidate || position - candidate > cMaximumOffset) {
            break;
        }
        if (candidate + cMinimumMatchLength <= input.size() &&
            readUInt32(input, candidate) == readUInt32(input, position)) {
            auto matchLength = cMinimumMatchLength;
            while (
                matchLength < maximumMatchLength && input[candidate + matchLength] == input[position + matchLength]) {
                ++matchLength;
            }
            if (matchLength > bestLength) {
                bestLength = matchLength;
                bestOffset = position - candidate;
            }
        }
        candidate = previous[candidate < previous.size() ? candidate : candidate & cMaximumOffset];
    }
    return {bestLength, bestOffset};
}

void Lz4Codec::throwMalformed(const text::String &message) {
    throw CompressionError{CompressionErrorReason::MalformedData, message};
}

auto Lz4Codec::readExtendedLength(const ConstByteSpan input, std::size_t &position, std::size_t length) -> std::size_t {
    while (true) {
        if (position >= input.size()) {
            throwMalformed("LZ4 length extension is truncated."_el);
        }
        const auto value = input[position++].toUInt8();
        if (length > std::numeric_limits<std::size_t>::max() - value) {
            throwMalformed("LZ4 length exceeds the supported range."_el);
        }
        length += value;
        if (value != cLengthExtensionMaximum) {
            return length;
        }
    }
}

auto Lz4Codec::decompressBlock(const ByteBlock &data, const DecompressionOptions &options) const -> ByteBlock {
    const auto input = data.span();
    if (!options.expectedOutputLength().has_value() || !options.expectedOutputLength()->isFinite()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Original length must be finite."_el};
    }
    const auto originalLength = *options.expectedOutputLength();
    if (input.empty()) {
        throwMalformed("LZ4 block is empty."_el);
    }
    auto output = ByteBlockEditor{};
    output.reserve(originalLength);
    auto inputPosition = std::size_t{};
    auto outputPosition = std::size_t{};
    const auto outputSize = originalLength.toSizeTOrThrow();
    while (inputPosition < input.size()) {
        const auto token = input[inputPosition++].toUInt8();
        auto literalLength = static_cast<std::size_t>(token >> 4U);
        if (literalLength == cLengthNibbleMaximum) {
            literalLength = readExtendedLength(input, inputPosition, literalLength);
        }
        if (literalLength > input.size() - inputPosition || literalLength > outputSize - outputPosition) {
            throwMalformed("LZ4 literal sequence exceeds its input or output."_el);
        }
        if (literalLength != 0U) {
            output.append(input.subspan(inputPosition, literalLength));
            inputPosition += literalLength;
            outputPosition += literalLength;
        }
        if (inputPosition == input.size()) {
            if ((token & 0x0fU) != 0U) {
                throwMalformed("LZ4 final literal sequence has match bits."_el);
            }
            break;
        }
        if (input.size() - inputPosition < 2U) {
            throwMalformed("LZ4 match offset is truncated."_el);
        }
        const auto offset = static_cast<std::size_t>(input[inputPosition].toUInt16()) |
            (static_cast<std::size_t>(input[inputPosition + 1U].toUInt16()) << 8U);
        inputPosition += 2U;
        if (offset == 0U || offset > outputPosition) {
            throwMalformed("LZ4 match offset is invalid."_el);
        }
        auto matchLength = static_cast<std::size_t>(token & 0x0fU) + cMinimumMatchLength;
        if ((token & 0x0fU) == cLengthNibbleMaximum) {
            matchLength = readExtendedLength(input, inputPosition, matchLength);
        }
        if (matchLength > outputSize - outputPosition) {
            throwMalformed("LZ4 match exceeds the expected output length."_el);
        }
        output.appendRepeated(
            ByteRange{ByteIndex::fromSizeT(outputPosition - offset), ByteCount::fromSizeT(offset)},
            ByteCount::fromSizeT(matchLength));
        outputPosition += matchLength;
    }
    if (outputPosition != outputSize) {
        throw CompressionError{
            CompressionErrorReason::LengthMismatch, "LZ4 output does not match the expected length."_el};
    }
    return output;
}

auto Lz4Codec::maximumPayloadLength(const ByteCount length) const -> ByteCount {
    if (!length.isFinite()) {
        throw OutOfRangeFailure{"Compression length must be finite."_el};
    }
    return length.addedOrThrow(ByteCount::fromSizeT(length.toSizeTOrThrow() / cLengthExtensionMaximum + cSizeOverhead));
}

}
