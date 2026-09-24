// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardDecoder.hpp"

#include "ZstandardReverseBitReader.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <limits>

namespace erbsland::compression::impl {

using namespace text::literals;

ZstandardDecoder::ZstandardDecoder(
    mem::ByteBlockEditor &output, const std::size_t windowSize, const std::size_t maximumOutputLength) noexcept :
    _output{output},
    _windowSize{windowSize},
    _maximumOutputLength{maximumOutputLength},
    _maximumBlockSize{std::min(windowSize, std::size_t{128U * 1024U})} {
}

void ZstandardDecoder::decodeBlock(const mem::ConstByteSpan data) {
    _blockBegin = _output.length().toSizeT();
    auto position = decodeLiterals(data);
    const auto sequenceCount = initializeSequences(data, position);
    if (sequenceCount == 0U) {
        if (position != data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Extraneous data after zero sequences."_el};
        }
        auto literalPosition = std::size_t{};
        appendLiterals(_literals.length().toSizeTOrThrow(), literalPosition);
        return;
    }
    executeSequences(data, position, sequenceCount);
}

auto ZstandardDecoder::decodeLiterals(const mem::ConstByteSpan data) -> std::size_t {
    if (data.empty()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals section is missing."_el};
    }
    _literals.clear();
    const auto header = data.front().toUInt8();
    return (header & 3U) < 2U ? decodeRawLiterals(data, header, 1U) : decodeHuffmanLiterals(data, header, 1U);
}

auto ZstandardDecoder::decodeRawLiterals(const mem::ConstByteSpan data, const uint8_t header, std::size_t position)
    -> std::size_t {
    const auto raw = (header & 3U) == 0U;
    std::size_t regeneratedSize;
    const auto sizeFormat = (header >> 2U) & 3U;
    if (sizeFormat == 0U || sizeFormat == 2U) {
        regeneratedSize = header >> 3U;
    } else if (sizeFormat == 1U) {
        if (position >= data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals header is truncated."_el};
        }
        regeneratedSize = (header >> 4U) + (data[position++].toUInt32() << 4U);
    } else {
        if (position + 2U > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals header is truncated."_el};
        }
        regeneratedSize = (header >> 4U) + (data[position].toUInt32() << 4U) + (data[position + 1U].toUInt32() << 12U);
        position += 2U;
    }
    if (regeneratedSize > _maximumBlockSize) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals exceed the block limit."_el};
    }
    _literals.reserve(unit::ByteLength::fromSizeT(regeneratedSize));
    if (raw) {
        if (regeneratedSize > data.size() - position) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Raw Zstandard literals are truncated."_el};
        }
        const auto literals = data.subspan(position, regeneratedSize);
        _literals.append(literals);
        return position + regeneratedSize;
    }
    if (position >= data.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard RLE literal byte is missing."_el};
    }
    _literals.append(data[position], unit::ByteLength::fromSizeT(regeneratedSize));
    return position + 1U;
}

auto ZstandardDecoder::decodeHuffmanLiterals(const mem::ConstByteSpan data, const uint8_t header, std::size_t position)
    -> std::size_t {
    std::size_t regeneratedSize;
    std::size_t compressedSize;
    auto streamCount = std::size_t{4U};
    const auto sizeFormat = (header >> 2U) & 3U;
    if (sizeFormat < 2U) {
        if (position + 2U > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals header is truncated."_el};
        }
        regeneratedSize = (header >> 4U) | ((data[position].toUInt32() & 0x3fU) << 4U);
        compressedSize = (data[position].toUInt32() >> 6U) | (data[position + 1U].toUInt32() << 2U);
        position += 2U;
        streamCount = sizeFormat == 0U ? 1U : 4U;
    } else if (sizeFormat == 2U) {
        if (position + 3U > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals header is truncated."_el};
        }
        regeneratedSize =
            (header >> 4U) | (data[position].toUInt32() << 4U) | ((data[position + 1U].toUInt32() & 3U) << 12U);
        compressedSize = (data[position + 1U].toUInt32() >> 2U) | (data[position + 2U].toUInt32() << 6U);
        position += 3U;
    } else {
        if (position + 4U > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard literals header is truncated."_el};
        }
        regeneratedSize =
            (header >> 4U) | (data[position].toUInt32() << 4U) | ((data[position + 1U].toUInt32() & 0x3fU) << 12U);
        compressedSize = (data[position + 1U].toUInt32() >> 6U) | (data[position + 2U].toUInt32() << 2U) |
            (data[position + 3U].toUInt32() << 10U);
        position += 4U;
    }
    if (regeneratedSize > _maximumBlockSize || compressedSize > data.size() - position) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard compressed literals size."_el};
    }
    const auto compressedEnd = position + compressedSize;
    if ((header & 3U) == 2U) {
        _huffmanTable.read(data.subspan(0U, compressedEnd), position);
    } else if (!_huffmanTable.isValid()) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Treeless literals have no previous Huffman table."_el};
    }
    if (position > compressedEnd) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Zstandard Huffman table exceeds its section."_el};
    }
    _literals.reserve(unit::ByteLength::fromSizeT(regeneratedSize));
    const auto streamSize = compressedEnd - position;
    if (streamCount == 1U) {
        _huffmanTable.decodeStream(data, position, compressedEnd, regeneratedSize, _literals);
    } else {
        decodeFourHuffmanStreams(data, position, streamSize, regeneratedSize);
    }
    return compressedEnd;
}

void ZstandardDecoder::decodeFourHuffmanStreams(
    const mem::ConstByteSpan data,
    const std::size_t position,
    const std::size_t size,
    const std::size_t regeneratedSize) {
    if (size < 6U || position > data.size() || size > data.size() - position) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman jump table is truncated."_el};
    }
    const auto read16 = [&data](const std::size_t offset) {
        return data[offset].toUInt32() | (data[offset + 1U].toUInt32() << 8U);
    };
    const auto size1 = static_cast<std::size_t>(read16(position));
    const auto size2 = static_cast<std::size_t>(read16(position + 2U));
    const auto size3 = static_cast<std::size_t>(read16(position + 4U));
    if (size1 + size2 + size3 > size - 6U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard Huffman stream sizes."_el};
    }
    const auto size4 = size - 6U - size1 - size2 - size3;
    const auto regeneratedStreamSize = (regeneratedSize + 3U) / 4U;
    if (regeneratedSize < regeneratedStreamSize * 3U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard Huffman regenerated size."_el};
    }
    auto streamPosition = position + 6U;
    for (
        const auto &[compressed, regenerated] : std::array{
            std::pair{size1, regeneratedStreamSize},
            std::pair{size2, regeneratedStreamSize},
            std::pair{size3, regeneratedStreamSize},
            std::pair{size4, regeneratedSize - regeneratedStreamSize * 3U}}) {
        _huffmanTable.decodeStream(data, streamPosition, streamPosition + compressed, regenerated, _literals);
        streamPosition += compressed;
    }
}

auto ZstandardDecoder::initializeSequences(const mem::ConstByteSpan data, std::size_t &position) -> std::size_t {
    if (position >= data.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard sequences section is missing."_el};
    }
    const auto header = data[position++].toUInt8();
    if (header == 0U) {
        return 0U;
    }
    std::size_t sequenceCount;
    if (header < 128U) {
        sequenceCount = header;
    } else if (header < 255U) {
        if (position >= data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard sequence count is truncated."_el};
        }
        sequenceCount = ((static_cast<std::size_t>(header) - 128U) << 8U) + data[position++].toUInt32();
    } else {
        if (position + 2U > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard sequence count is truncated."_el};
        }
        sequenceCount = data[position].toUInt32() + (data[position + 1U].toUInt32() << 8U) + 0x7f00U;
        position += 2U;
    }
    if (position >= data.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard sequence modes are missing."_el};
    }
    const auto modes = data[position++].toUInt8();
    if ((modes & 3U) != 0U) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Reserved Zstandard sequence mode bits are set."_el};
    }
    initializeSequenceTable(data, position, ZstandardSequenceCode::LiteralLength, modes >> 6U, _literalLengthTable);
    initializeSequenceTable(data, position, ZstandardSequenceCode::Offset, (modes >> 4U) & 3U, _offsetTable);
    initializeSequenceTable(data, position, ZstandardSequenceCode::MatchLength, (modes >> 2U) & 3U, _matchLengthTable);
    return sequenceCount;
}

void ZstandardDecoder::initializeSequenceTable(
    const mem::ConstByteSpan data,
    std::size_t &position,
    const ZstandardSequenceCode kind,
    const uint8_t mode,
    ZstandardSequenceTable &table) {
    if (mode == 0U) {
        table.setPredefined(kind);
    } else if (mode == 1U) {
        if (position >= data.size()) {
            throw CompressionError{
                CompressionErrorReason::MalformedData, "Zstandard RLE sequence symbol is missing."_el};
        }
        table.setRle(kind, data[position++].toUInt8());
    } else if (mode == 2U) {
        table.readCompressed(data, position, kind);
    } else if (!table.isValid()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard repeat sequence table is missing."_el};
    }
}

void ZstandardDecoder::executeSequences(
    const mem::ConstByteSpan data, const std::size_t position, const std::size_t sequenceCount) {
    auto reader = ZstandardReverseBitReader{data, position, data.size()};
    auto literalState = reader.read(_literalLengthTable.accuracyLog());
    auto offsetState = reader.read(_offsetTable.accuracyLog());
    auto matchState = reader.read(_matchLengthTable.accuracyLog());
    auto literalPosition = std::size_t{};
    for (auto sequence = std::size_t{}; sequence < sequenceCount; ++sequence) {
        const auto &offsetEntry = _offsetTable.entry(offsetState);
        const auto &matchEntry = _matchLengthTable.entry(matchState);
        const auto &literalEntry = _literalLengthTable.entry(literalState);
        const auto encodedOffset = offsetEntry.baseline + reader.read(offsetEntry.valueBits);
        const auto matchLength = matchEntry.baseline + reader.read(matchEntry.valueBits);
        const auto literalLength = literalEntry.baseline + reader.read(literalEntry.valueBits);
        const auto offset = resolveOffset(encodedOffset, offsetEntry.valueBits, literalLength);
        if (sequence + 1U < sequenceCount) {
            literalState = literalEntry.stateBase + reader.read(literalEntry.stateBits);
            matchState = matchEntry.stateBase + reader.read(matchEntry.stateBits);
            offsetState = offsetEntry.stateBase + reader.read(offsetEntry.stateBits);
        }
        appendLiterals(literalLength, literalPosition);
        appendMatch(offset, matchLength);
    }
    appendLiterals(_literals.length().toSizeTOrThrow() - literalPosition, literalPosition);
    if (!reader.isAtEnd()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Extraneous bits after Zstandard sequences."_el};
    }
}

auto ZstandardDecoder::resolveOffset(uint32_t encodedOffset, const uint8_t offsetBits, const uint32_t literalLength)
    -> uint32_t {
    if (offsetBits > 1U) {
        _repeatOffsets[2] = _repeatOffsets[1];
        _repeatOffsets[1] = _repeatOffsets[0];
        _repeatOffsets[0] = encodedOffset;
        return encodedOffset;
    }
    if (literalLength == 0U) {
        ++encodedOffset;
    }
    if (encodedOffset == 1U) {
        return _repeatOffsets[0];
    }
    if (encodedOffset == 2U) {
        const auto result = _repeatOffsets[1];
        _repeatOffsets[1] = _repeatOffsets[0];
        _repeatOffsets[0] = result;
        return result;
    }
    if (encodedOffset == 3U) {
        const auto result = _repeatOffsets[2];
        _repeatOffsets[2] = _repeatOffsets[1];
        _repeatOffsets[1] = _repeatOffsets[0];
        _repeatOffsets[0] = result;
        return result;
    }
    if (encodedOffset == 4U && _repeatOffsets[0] > 1U) {
        const auto result = _repeatOffsets[0] - 1U;
        _repeatOffsets[2] = _repeatOffsets[1];
        _repeatOffsets[1] = _repeatOffsets[0];
        _repeatOffsets[0] = result;
        return result;
    }
    throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard repeat offset."_el};
}

void ZstandardDecoder::appendLiterals(const std::size_t count, std::size_t &literalPosition) {
    const auto literalSize = _literals.length().toSizeTOrThrow();
    if (literalPosition > literalSize || count > literalSize - literalPosition) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Zstandard sequence consumes too many literals."_el};
    }
    requireOutputSpace(count);
    _output.append(_literals.span().subspan(literalPosition, count));
    literalPosition += count;
}

void ZstandardDecoder::appendMatch(const uint32_t offset, const uint32_t length) {
    const auto outputLength = _output.length().toSizeT();
    const auto availableHistory = std::min(outputLength, _windowSize);
    if (offset == 0U || offset > availableHistory) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard match offset exceeds the window."_el};
    }
    requireOutputSpace(length);
    _output.appendRepeated(
        unit::ByteRange{unit::ByteIndex::fromSizeT(outputLength - offset), unit::ByteLength::fromSizeT(offset)},
        unit::ByteLength::fromSizeT(length));
}

void ZstandardDecoder::requireOutputSpace(const std::size_t count) const {
    const auto outputLength = _output.length().toSizeT();
    if (outputLength > _maximumOutputLength || count > _maximumOutputLength - outputLength) {
        throw err::OutOfRangeError{"Zstandard output exceeds the configured maximum."_el};
    }
    if (outputLength - _blockBegin > _maximumBlockSize || count > _maximumBlockSize - (outputLength - _blockBegin)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard block regenerates too many bytes."_el};
    }
}

}
