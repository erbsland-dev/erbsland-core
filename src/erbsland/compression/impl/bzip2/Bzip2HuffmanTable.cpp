// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bzip2HuffmanTable.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <array>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

Bzip2HuffmanTable::Bzip2HuffmanTable(const std::vector<uint8_t> &lengths) {
    auto counts = std::array<uint16_t, cMaximumCodeLength + 1U>{};
    for (const auto length : lengths) {
        if (length < 1U || length > cMaximumCodeLength) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 Huffman length is invalid."_el};
        }
        ++counts[length];
        _maximumLength = std::max(_maximumLength, length);
    }
    if (_maximumLength == 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 Huffman tree is empty."_el};
    }
    auto next = std::array<uint32_t, cMaximumCodeLength + 1U>{};
    auto code = uint32_t{};
    for (auto length = 1U; length <= cMaximumCodeLength; ++length) {
        code = (code + counts[length - 1U]) << 1U;
        next[length] = code;
        if (code + counts[length] > (uint32_t{1U} << length)) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 Huffman tree is oversubscribed."_el};
        }
    }
    _symbols.reserve(lengths.size());
    for (auto length = std::size_t{1U}; length <= _maximumLength; ++length) {
        _firstCodes[length] = next[length];
        _firstSymbols[length] = static_cast<uint16_t>(_symbols.size());
        _symbolCounts[length] = counts[length];
        for (auto symbol = std::size_t{}; symbol < lengths.size(); ++symbol) {
            if (lengths[symbol] == length) {
                _symbols.push_back(static_cast<uint16_t>(symbol));
            }
        }
    }
}

[[nodiscard]] auto Bzip2HuffmanTable::decode(CodecBitReader &input) const -> uint16_t {
    auto &reader = input.buffered(_maximumLength);
    const auto available = std::min<std::size_t>(_maximumLength, reader.remainingBitCount());
    if (available == 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 bit stream is truncated."_el};
    }
    const auto position = reader.bitPosition();
    const auto bits = static_cast<uint32_t>(reader.readBits(available));
    for (auto length = std::size_t{1U}; length <= available; ++length) {
        const auto code = bits >> (available - length);
        const auto firstCode = _firstCodes[length];
        if (code >= firstCode && code - firstCode < _symbolCounts[length]) {
            reader.setBitPosition(position + length);
            return _symbols.at(_firstSymbols[length] + code - firstCode);
        }
    }
    throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 Huffman code is invalid."_el};
}

}
