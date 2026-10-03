// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardFrame.hpp"

#include "ZstandardChecksum.hpp"
#include "ZstandardDecoder.hpp"
#include "ZstandardEncoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <optional>

namespace erbsland::compression::impl {

using namespace err;
using namespace mem;
using namespace unit;
using namespace text::literals;

[[nodiscard]] auto ZstandardFrame::windowLogFor(const CompressionLevel level) noexcept -> unsigned {
    return cWindowLogs[static_cast<std::size_t>(level)];
}

[[nodiscard]] auto ZstandardFrame::isRepeated(const ConstByteSpan input) noexcept -> bool {
    return std::ranges::all_of(input, [value = input.front()](const Byte byte) { return byte == value; });
}

void ZstandardFrame::appendLittle(ByteBlockEditor &output, uint64_t value, const std::size_t count) {
    for (auto index = std::size_t{}; index < count; ++index) {
        output.append(Byte::fromCroppedUInt64(value));
        value >>= 8U;
    }
}

[[noreturn]] void ZstandardFrame::malformed(const text::String &message) {
    throw CompressionError{CompressionErrorReason::MalformedData, message};
}

void ZstandardFrame::compressStream(CodecReader &input, CompressionLevel level, const CodecOutput::Write &sink) const {
    const auto windowLog = windowLogFor(level);
    const auto window = std::size_t{1U} << windowLog;
    auto header = ByteBlockEditor{};
    appendLittle(header, cMagic, 4U);
    header.append(Byte{cChecksumFlag});
    header.append(Byte::fromCroppedUInt32((windowLog - cWindowLogBase) << 3U));
    sink(header.span());
    auto history = ByteBlockEditor{};
    history.reserve(ByteCount{65536U});
    ZstandardChecksum checksum;
    auto encoder = ZstandardEncoder{{}, level, window};
    std::size_t removed{};
    do {
        const auto block = input.block(cMaximumBlockSize);
        checksum.update(block.span());
        const auto historyLength = history.length().toSizeT();
        history.append(block.span());
        encoder.setInput(history.span(), removed);
        const auto compressed = encoder.encodeBlock(historyLength, block.length().toSizeT());
        const auto repeated = block.length().toRawValue() > 1U && isRepeated(block.span());
        const auto useCompressed = !repeated && compressed && compressed->size() < block.length().toSizeT();
        const auto size = useCompressed ? compressed->size() : block.length().toSizeT();
        const auto type = repeated ? 2U : (useCompressed ? 4U : 0U);
        header.clear();
        appendLittle(header, (size << 3U) | type | (input.atEnd() ? 1U : 0U), 3U);
        sink(header.span());
        if (repeated) {
            sink(block.span().first(1U));
        } else if (useCompressed) {
            sink(ConstByteSpan{*compressed});
        } else {
            sink(block.span());
        }
        removed = history.length().toSizeT() - std::min(history.length().toSizeT(), window);
        history.remove(ByteRange{ByteIndex{}, ByteCount::fromSizeT(removed)});
    } while (!input.atEnd());
    header.clear();
    appendLittle(header, checksum.value(), 4U);
    sink(header.span());
}

void ZstandardFrame::decompressStream(
    CodecReader &input, const DecompressionOptions &options, const CodecOutput::Write &sink) const {
    const auto magic = static_cast<uint32_t>(input.little(4U));
    if ((magic & cSkippableMagicMask) == cSkippableMagic) {
        throw CompressionError{
            CompressionErrorReason::UnsupportedFeature, "Zstandard skippable frames are unsupported."_el};
    }
    if (magic != cMagic) {
        malformed("Invalid Zstandard frame magic."_el);
    }
    const auto descriptor = static_cast<uint8_t>(input.little(1U));
    if ((descriptor & cReservedFlag) != 0U) {
        malformed("Reserved Zstandard frame-header bit is set."_el);
    }
    const auto singleSegment = (descriptor & cSingleSegmentFlag) != 0U;
    const auto checksum = (descriptor & cChecksumFlag) != 0U;
    const auto dictionaryFlag = descriptor & cDictionaryFlagMask;
    auto windowSize = ByteCount{};
    if (!singleSegment) {
        const auto windowDescriptor = static_cast<uint8_t>(input.little(1U));
        const auto windowLog = static_cast<unsigned>((windowDescriptor >> 3U) + cWindowLogBase);
        const auto base = ByteCount{uint64_t{1U} << windowLog, ByteCount::Unchecked{}};
        windowSize = base + (base / 8U) * (windowDescriptor & 7U);
    }
    const auto dictionaryFieldSize = dictionaryFlag == 3U ? std::size_t{4U} : static_cast<std::size_t>(dictionaryFlag);
    const auto dictionaryId = input.little(dictionaryFieldSize);
    if (dictionaryId != 0U) {
        throw CompressionError{
            CompressionErrorReason::UnsupportedFeature, "External Zstandard dictionaries are unsupported."_el};
    }
    const auto sizeFlag = descriptor >> 6U;
    auto contentFieldSize = std::size_t{};
    if (sizeFlag == 0U) {
        contentFieldSize = singleSegment ? 1U : 0U;
    } else {
        contentFieldSize = sizeFlag == 1U ? 2U : (sizeFlag == 2U ? 4U : 8U);
    }
    auto contentSize = std::optional<std::size_t>{};
    if (contentFieldSize != 0U) {
        auto value = input.little(contentFieldSize);
        if (contentFieldSize == 2U) {
            value += cTwoByteContentSizeBase;
        }
        if (value > std::numeric_limits<std::size_t>::max()) {
            throw OutOfRangeError{"Zstandard content size exceeds this platform."_el};
        }
        contentSize = static_cast<std::size_t>(value);
        if (singleSegment) {
            windowSize = ByteCount::fromInteger(value);
        }
    }
    if (windowSize > options.maximumWorkspaceLength() ||
        options.maximumWorkspaceLength() - windowSize < ByteCount{2U * 1024U * 1024U}) {
        throw OutOfRangeError{"Zstandard window exceeds the configured workspace limit."_el};
    }
    const auto maximum = options.maximumOutputLength();
    if (contentSize.has_value() && ByteCount::fromSizeT(*contentSize) > maximum) {
        throw OutOfRangeError{"Zstandard content exceeds the configured output limit."_el};
    }
    auto output = ByteBlockEditor{};
    const auto windowLength = windowSize;
    output.reserve(windowLength + ByteCount::fromSizeT(cMaximumBlockSize));
    ByteCount total{};
    ZstandardChecksum checksumValue;
    auto decoder = ZstandardDecoder{output, windowSize.toSizeTOrThrow(), std::numeric_limits<std::size_t>::max()};
    const auto maximumBlockOutput = std::min(cMaximumBlockSize, windowSize.toSizeTOrThrow());
    auto last = false;
    while (!last) {
        const auto begin = output.length().toSizeT();
        const auto header = static_cast<uint32_t>(input.little(3U));
        last = (header & 1U) != 0U;
        const auto blockType = (header >> 1U) & 3U;
        const auto blockSize = static_cast<std::size_t>(header >> 3U);
        if (blockSize > cMaximumBlockSize) {
            malformed("Zstandard block exceeds 128 KiB."_el);
        }
        if (blockType == 0U) {
            if (blockSize > maximumBlockOutput) {
                malformed("Raw Zstandard block exceeds the frame window."_el);
            }
            if (ByteCount::fromSizeT(blockSize) > maximum - total) {
                throw OutOfRangeError{"Zstandard output exceeds the configured maximum."_el};
            }
            const auto block = input.block(blockSize);
            if (block.length().toSizeT() != blockSize) {
                malformed("Truncated Zstandard block."_el);
            }
            output.append(block.span());
        } else if (blockType == 1U) {
            if (blockSize > maximumBlockOutput) {
                malformed("RLE Zstandard block exceeds the frame window."_el);
            }
            if (ByteCount::fromSizeT(blockSize) > maximum - total) {
                throw OutOfRangeError{"Zstandard output exceeds the configured maximum."_el};
            }
            output.append(input.byte(), ByteCount::fromSizeT(blockSize));
        } else if (blockType == 2U) {
            const auto block = input.block(blockSize);
            if (block.length().toSizeT() != blockSize) {
                malformed("Truncated Zstandard block."_el);
            }
            decoder.decodeBlock(block.span());
        } else {
            malformed("Reserved Zstandard block type."_el);
        }
        const auto produced = output.span().subspan(begin);
        if (ByteCount::fromSizeT(produced.size()) > maximum - total) {
            throw OutOfRangeError{"Zstandard output limit exceeded."_el};
        }
        total += ByteCount::fromSizeT(produced.size());
        checksumValue.update(produced);
        sink(produced);
        if (output.length() > windowLength) {
            output.remove(ByteRange{ByteIndex::zero(), output.length() - windowLength});
        }
    }
    if (contentSize.has_value() && total != ByteCount::fromSizeT(*contentSize)) {
        throw CompressionError{
            CompressionErrorReason::LengthMismatch, "Zstandard frame content size does not match."_el};
    }
    if (options.expectedOutputLength().has_value() && total != *options.expectedOutputLength()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Zstandard output length does not match."_el};
    }
    if (checksum) {
        const auto stored = static_cast<uint32_t>(input.little(4U));
        const auto actual = static_cast<uint32_t>(checksumValue.value());
        if (stored != actual) {
            malformed("Zstandard content checksum does not match."_el);
        }
    }
    input.requireEnd();
}

}
