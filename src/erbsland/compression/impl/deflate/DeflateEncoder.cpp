// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DeflateEncoder.hpp"

#include "DeflateHuffmanTable.hpp"
#include "DeflateTables.hpp"

#include "../CodecBitOutput.hpp"

#include "../../../mem/ByteBlockEditor.hpp"

#include <algorithm>
#include <vector>

namespace erbsland::compression::impl {

DeflateEncoder::DeflateEncoder(const CompressionLevel level) : _level{level} {
}

[[nodiscard]] auto DeflateEncoder::reverseBits(const uint32_t value, const unsigned count) noexcept -> uint16_t {
    return DeflateHuffmanTable::reverseBits(value, count);
}

void DeflateEncoder::fixedCode(const uint16_t symbol, uint16_t &code, unsigned &length) noexcept {
    if (symbol <= 143U) {
        code = static_cast<uint16_t>(0x30U + symbol);
        length = 8U;
    } else if (symbol <= 255U) {
        code = static_cast<uint16_t>(0x190U + symbol - 144U);
        length = 9U;
    } else if (symbol <= 279U) {
        code = static_cast<uint16_t>(symbol - 256U);
        length = 7U;
    } else {
        code = static_cast<uint16_t>(0xc0U + symbol - 280U);
        length = 8U;
    }
}

void DeflateEncoder::writeFixedSymbol(const uint16_t symbol) {
    auto code = uint16_t{};
    auto length = unsigned{};
    fixedCode(symbol, code, length);
    _writer.writeBits(reverseBits(code, length), length);
}

[[nodiscard]] auto DeflateEncoder::chainDepth() const noexcept -> std::size_t {
    switch (_level) {
    case CompressionLevel::Fastest:
        return 0U;
    case CompressionLevel::Fast:
        return 4U;
    case CompressionLevel::Default:
        return 16U;
    case CompressionLevel::High:
        return 64U;
    case CompressionLevel::Highest:
        return 256U;
    }
    return 16U;
}

[[nodiscard]] auto DeflateEncoder::hash(const mem::ConstByteSpan input, const std::size_t position) noexcept
    -> std::size_t {
    const auto value =
        (input[position].toUInt32() << 16U) | (input[position + 1U].toUInt32() << 8U) | input[position + 2U].toUInt32();
    return (value * cHashMultiplier) >> 17U;
}

void DeflateEncoder::encodeFixed() {
    _writer.writeBits(_final ? 1U : 0U, 1U);
    _writer.writeBits(1U, 2U);
    auto head = std::vector<std::size_t>(cHashTableSize, cNoPosition);
    auto previous = std::vector<std::size_t>(std::min(_input.size(), cChainTableSize), cNoPosition);
    // The mask also preserves all indexes when the input is shorter than the circular table.
    for (std::size_t i{}; i < _start && i + 3U <= _input.size(); ++i) {
        const auto h = hash(_input, i);
        previous[i & (cChainTableSize - 1U)] = head[h];
        head[h] = i;
    }
    auto position = _start;
    while (position < _input.size()) {
        auto bestLength = std::size_t{};
        auto bestDistance = std::size_t{};
        if (position + cMinimumMatchLength <= _input.size()) {
            const auto hashValue = hash(_input, position);
            auto candidate = head[hashValue];
            previous[position & (cChainTableSize - 1U)] = candidate;
            head[hashValue] = position;
            for (auto depth = std::size_t{}; candidate != cNoPosition && depth < chainDepth(); ++depth) {
                // Hash-chain links contain earlier positions, including when circular slots are reused.
                if (position - candidate > cMaximumMatchDistance) {
                    break;
                }
                auto length = std::size_t{};
                const auto maximum = std::min(cMaximumMatchLength, _input.size() - position);
                while (length < maximum && _input[candidate + length] == _input[position + length]) {
                    ++length;
                }
                if (length >= cMinimumMatchLength && length > bestLength) {
                    bestLength = length;
                    bestDistance = position - candidate;
                }
                candidate = previous[candidate & (cChainTableSize - 1U)];
            }
        }
        if (bestLength < cMinimumMatchLength) {
            writeFixedSymbol(_input[position].toUInt8());
            ++position;
            continue;
        }
        writeLength(bestLength);
        writeDistance(bestDistance);
        for (
            auto index = std::size_t{1U}; index < bestLength && position + index + cMinimumMatchLength <= _input.size();
            ++index) {
            const auto chainPosition = position + index;
            const auto hashValue = hash(_input, chainPosition);
            previous[chainPosition & (cChainTableSize - 1U)] = head[hashValue];
            head[hashValue] = chainPosition;
        }
        position += bestLength;
    }
    writeFixedSymbol(cEndOfBlockSymbol);
}

void DeflateEncoder::writeLength(const std::size_t length) {
    for (auto index = std::size_t{}; index < deflate_tables::cLengthBase.size(); ++index) {
        const auto maximum = index + 1U < deflate_tables::cLengthBase.size()
            ? deflate_tables::cLengthBase[index + 1U] - 1U
            : cMaximumMatchLength;
        if (length <= maximum) {
            writeFixedSymbol(static_cast<uint16_t>(257U + index));
            _writer.writeBits(
                static_cast<uint32_t>(length - deflate_tables::cLengthBase[index]),
                deflate_tables::cLengthExtra[index]);
            return;
        }
    }
}

void DeflateEncoder::writeDistance(const std::size_t distance) {
    for (auto index = std::size_t{}; index < deflate_tables::cDistanceBase.size(); ++index) {
        const auto maximum =
            deflate_tables::cDistanceBase[index] + ((std::size_t{1U} << deflate_tables::cDistanceExtra[index]) - 1U);
        if (distance <= maximum) {
            _writer.writeBits(reverseBits(static_cast<uint32_t>(index), 5U), 5U);
            _writer.writeBits(
                static_cast<uint32_t>(distance - deflate_tables::cDistanceBase[index]),
                deflate_tables::cDistanceExtra[index]);
            return;
        }
    }
}

void DeflateEncoder::encodeStream(CodecReader &input, const CodecOutput::Write &output) {
    auto history = mem::ByteBlockEditor{};
    history.reserve(unit::ByteLength{65536U});
    while (!input.atEnd()) {
        const auto block = input.block(cMaximumStoredLength);
        const auto historyLength = history.length().toSizeT();
        history.append(block.span());
        _input = history.span();
        _start = historyLength;
        _final = input.atEnd();
        _writer.reset();
        if (_level != CompressionLevel::Fastest) {
            encodeFixed();
            // An empty stored block aligns the next bounded block without ending the stream.
            if (!_final) {
                _writer.writeBits(0U, 3U);
                _writer.alignToByte();
                _writer.writeBits(0U, 16U);
                _writer.writeBits(65535U, 16U);
            }
        }
        if (_level == CompressionLevel::Fastest || _writer.byteCount() > block.length().toSizeT() + 5U) {
            _writer.reset();
            _writer.writeBits(_final ? 1U : 0U, 3U);
            _writer.alignToByte();
            const auto length = block.length().toRawValue();
            _writer.writeBits(length, 16U);
            _writer.writeBits(length ^ 65535U, 16U);
            _writer.writeBytes(block.span());
        }
        codecBitOutput::drain(_writer, output, _final);
        if (_final) {
            return;
        }
        const auto removed = history.length().toSizeT() - std::min(history.length().toSizeT(), cMaximumMatchDistance);
        history.remove(unit::ByteRange{unit::ByteIndex{}, unit::ByteLength::fromSizeT(removed)});
    }
    _writer.reset();
    _writer.writeBits(1U, 3U);
    _writer.alignToByte();
    _writer.writeBits(0U, 16U);
    _writer.writeBits(65535U, 16U);
    codecBitOutput::drain(_writer, output, true);
}

}
