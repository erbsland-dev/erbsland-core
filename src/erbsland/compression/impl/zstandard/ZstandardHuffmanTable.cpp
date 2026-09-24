// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardHuffmanTable.hpp"

#include "ZstandardFseTable.hpp"
#include "ZstandardReverseBitReader.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <array>
#include <bit>

namespace erbsland::compression::impl {

using namespace text::literals;

void ZstandardHuffmanTable::read(const mem::ConstByteSpan data, std::size_t &position) {
    if (position >= data.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman table is truncated."_el};
    }
    const auto header = data[position++].toUInt8();
    auto weights = std::array<uint8_t, 256U>{};
    auto count = std::size_t{};
    if (header < 128U) {
        const auto end = position + header;
        if (end < position || end > data.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman table is truncated."_el};
        }
        count = readCompressedWeights(data, position, end, weights);
        position = end;
    } else {
        count = static_cast<std::size_t>(header) - 127U;
        const auto byteCount = (count + 1U) / 2U;
        if (byteCount > data.size() - position) {
            throw CompressionError{
                CompressionErrorReason::MalformedData, "Zstandard Huffman weights are truncated."_el};
        }
        for (auto index = std::size_t{}; index < count; index += 2U) {
            const auto value = data[position++].toUInt8();
            weights[index] = value >> 4U;
            if (index + 1U < weights.size()) {
                weights[index + 1U] = value & 0x0fU;
            }
        }
    }
    build(weights, count);
}

void ZstandardHuffmanTable::decodeStream(
    const mem::ConstByteSpan data,
    const std::size_t begin,
    const std::size_t end,
    const std::size_t regeneratedSize,
    mem::ByteBuffer &output) const {
    if (!isValid()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Missing Zstandard Huffman table."_el};
    }
    auto reader = ZstandardReverseBitReader{data, begin, end};
    output.reserve(output.length().addedOrThrow(unit::ByteLength::fromSizeT(regeneratedSize)));
    for (auto index = std::size_t{}; index < regeneratedSize; ++index) {
        const auto lookup = reader.peekPadded(_tableBits);
        const auto &entry = _entries[lookup];
        if (entry.bitCount == 0U || reader.remainingBitCount() < entry.bitCount) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman stream is truncated."_el};
        }
        output.append(mem::Byte{entry.symbol});
        reader.discard(entry.bitCount);
    }
    if (!reader.isAtEnd()) {
        throw CompressionError{
            CompressionErrorReason::MalformedData, "Extraneous bits in Zstandard Huffman stream."_el};
    }
}

auto ZstandardHuffmanTable::readCompressedWeights(
    const mem::ConstByteSpan data, const std::size_t begin, const std::size_t end, std::array<uint8_t, 256U> &weights)
    -> std::size_t {
    auto tablePosition = begin;
    auto table = ZstandardFseTable{};
    table.read(data.first(end), tablePosition, 255U, 6U);
    if (tablePosition >= end) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman weight stream is empty."_el};
    }
    auto reader = ZstandardReverseBitReader{data, tablePosition, end};
    auto state1 = reader.read(table.accuracyLog());
    auto state2 = reader.read(table.accuracyLog());
    auto count = std::size_t{};
    while (true) {
        const auto &entry1 = table.entry(state1);
        if (reader.remainingBitCount() < entry1.bitCount) {
            if (count > 253U) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Too many Zstandard Huffman weights."_el};
            }
            weights[count++] = entry1.symbol;
            weights[count++] = table.entry(state2).symbol;
            break;
        }
        state1 = static_cast<uint32_t>(entry1.stateBase) + reader.read(entry1.bitCount);
        if (count >= 255U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Too many Zstandard Huffman weights."_el};
        }
        weights[count++] = entry1.symbol;
        const auto &entry2 = table.entry(state2);
        if (reader.remainingBitCount() < entry2.bitCount) {
            if (count > 253U) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Too many Zstandard Huffman weights."_el};
            }
            weights[count++] = entry2.symbol;
            weights[count++] = table.entry(state1).symbol;
            break;
        }
        state2 = static_cast<uint32_t>(entry2.stateBase) + reader.read(entry2.bitCount);
        if (count >= 255U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Too many Zstandard Huffman weights."_el};
        }
        weights[count++] = entry2.symbol;
    }
    return count;
}

void ZstandardHuffmanTable::build(const std::array<uint8_t, 256U> &weights, const std::size_t count) {
    if (count == 0U || count >= weights.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard Huffman weight count."_el};
    }
    auto weightCounts = std::array<uint32_t, 13U>{};
    auto weightSum = uint32_t{};
    for (auto index = std::size_t{}; index < count; ++index) {
        const auto weight = weights[index];
        if (weight > 12U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman weight is too large."_el};
        }
        ++weightCounts[weight];
        if (weight != 0U) {
            weightSum += uint32_t{1U} << (weight - 1U);
        }
    }
    if (weightSum == 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman tree has no symbols."_el};
    }
    const auto tableBits = static_cast<std::size_t>(std::bit_width(weightSum));
    if (tableBits > 11U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard Huffman table is too large."_el};
    }
    const auto missingWeight = (uint32_t{1U} << tableBits) - weightSum;
    if (missingWeight == 0U || !std::has_single_bit(missingWeight)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard Huffman weight sum."_el};
    }
    auto completeWeights = weights;
    const auto finalWeight = static_cast<uint8_t>(std::bit_width(missingWeight));
    completeWeights[count] = finalWeight;
    ++weightCounts[finalWeight];
    if (weightCounts[1U] < 2U || (weightCounts[1U] & 1U) != 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard Huffman terminal weights."_el};
    }
    auto next = uint32_t{};
    for (auto weight = std::size_t{1U}; weight <= tableBits; ++weight) {
        const auto current = next;
        next += weightCounts[weight] << (weight - 1U);
        weightCounts[weight] = current;
    }
    _entries.fill({});
    for (auto symbol = std::size_t{}; symbol <= count; ++symbol) {
        const auto weight = completeWeights[symbol];
        if (weight == 0U) {
            continue;
        }
        const auto length = uint32_t{1U} << (weight - 1U);
        const auto bitCount = static_cast<uint8_t>(tableBits + 1U - weight);
        const auto start = weightCounts[weight];
        for (auto index = uint32_t{}; index < length; ++index) {
            _entries[start + index] = ZstandardHuffmanEntry{static_cast<uint8_t>(symbol), bitCount};
        }
        weightCounts[weight] += length;
    }
    _tableBits = static_cast<uint8_t>(tableBits);
}

}
