// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DeflateHuffmanTable.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

DeflateHuffmanTable::DeflateHuffmanTable(const std::vector<uint8_t> &lengths) {
    for (const auto length : lengths) {
        _maximumLength = std::max(_maximumLength, length);
    }
    if (_maximumLength == 0U || _maximumLength > cMaximumCodeLength) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate Huffman tree is empty or invalid."_el};
    }
    auto counts = std::array<uint16_t, cMaximumCodeLength + 1U>{};
    for (const auto length : lengths) {
        if (length > cMaximumCodeLength) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate code length is invalid."_el};
        }
        if (length != 0U) {
            ++counts[length];
        }
    }
    auto available = int32_t{1};
    for (auto length = 1U; length <= cMaximumCodeLength; ++length) {
        available = available * 2 - counts[length];
        if (available < 0) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate Huffman tree is oversubscribed."_el};
        }
    }
    auto nextCode = std::array<uint16_t, cMaximumCodeLength + 1U>{};
    auto code = uint16_t{};
    for (auto bits = 1U; bits <= cMaximumCodeLength; ++bits) {
        code = static_cast<uint16_t>((code + counts[bits - 1U]) << 1U);
        nextCode[bits] = code;
    }
    _entries.resize(std::size_t{1U} << _maximumLength);
    for (auto symbol = std::size_t{}; symbol < lengths.size(); ++symbol) {
        const auto length = lengths[symbol];
        if (length == 0U) {
            continue;
        }
        const auto reversed = reverseBits(nextCode[length]++, length);
        const auto repetitions = std::size_t{1U} << (_maximumLength - length);
        for (auto suffix = std::size_t{}; suffix < repetitions; ++suffix) {
            _entries[reversed | (suffix << length)] = DeflateHuffmanEntry{static_cast<uint16_t>(symbol), length};
        }
    }
}

[[nodiscard]] auto DeflateHuffmanTable::decode(CodecBitReader &input) const -> uint16_t {
    auto &reader = input.buffered(_maximumLength);
    const auto available = std::min<std::size_t>(_maximumLength, reader.remainingBitCount());
    if (available == 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate bit stream is truncated."_el};
    }
    const auto position = reader.bitPosition();
    const auto value = static_cast<std::size_t>(reader.readBits(available));
    const auto &entry = _entries[value];
    if (entry.length != 0U && entry.length <= available) {
        reader.setBitPosition(position + entry.length);
        return entry.symbol;
    }
    throw CompressionError{CompressionErrorReason::MalformedData, "Deflate Huffman code is invalid."_el};
}

[[nodiscard]] auto DeflateHuffmanTable::reverseBits(uint32_t value, const unsigned count) noexcept -> uint16_t {
    auto result = uint16_t{};
    for (auto index = 0U; index < count; ++index) {
        result = static_cast<uint16_t>((static_cast<uint32_t>(result) << 1U) | (value & 1U));
        value >>= 1U;
    }
    return result;
}

}
