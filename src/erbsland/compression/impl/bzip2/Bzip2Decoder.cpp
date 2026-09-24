// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bzip2Decoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <array>
#include <bit>
#include <numeric>
#include <utility>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

Bzip2Decoder::Bzip2Decoder(CodecReader &input, const DecompressionOptions &options, CodecOutput::Write output) :
    _reader{input}, _options{options}, _output{std::move(output), 0U, options.maximumOutputLength()} {
}

void Bzip2Decoder::decode() {
    if (_reader.readBits(24U) != cStreamMagic) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 stream header is invalid."_el};
    }
    const auto levelByte = _reader.readBits(8U);
    if (levelByte < '1' || levelByte > '9') {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block-size level is invalid."_el};
    }
    _blockSize = static_cast<std::size_t>(levelByte - '0') * cBlockSizeStep;
    const auto workspace = unit::ByteLength::fromSizeT(_blockSize * cWorkspaceFactor + cWorkspaceOverhead);
    if (workspace > _options.maximumWorkspaceLength()) {
        throw err::OutOfRangeError{"Bzip2 workspace exceeds the configured maximum."_el};
    }
    while (true) {
        const auto marker = _reader.readBits(48U);
        if (marker == cEndMarker) {
            const auto expectedCrc = _reader.readBits(32U);
            if (expectedCrc != _combinedCrc) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 combined CRC is invalid."_el};
            }
            break;
        }
        if (marker != cBlockMarker) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block marker is invalid."_el};
        }
        decodeBlock();
    }
    _reader.requireEnd();
    if (_options.expectedOutputLength().has_value() && _output.length() != *_options.expectedOutputLength()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Bzip2 output length does not match."_el};
    }
    _output.flush();
}

void Bzip2Decoder::append(const uint8_t value, const std::size_t count) {
    _output.append(mem::Byte{value}, unit::ByteLength::fromSizeT(count));
}

auto Bzip2Decoder::readAlphabet() -> std::vector<uint8_t> {
    auto groups = std::array<bool, cAlphabetGroupCount>{};
    for (auto &group : groups) {
        group = _reader.readBits(1U) != 0U;
    }
    auto alphabet = std::vector<uint8_t>{};
    for (auto group = std::size_t{}; group < groups.size(); ++group) {
        if (!groups[group]) {
            continue;
        }
        for (auto index = std::size_t{}; index < cAlphabetGroupSize; ++index) {
            if (_reader.readBits(1U) != 0U) {
                alphabet.push_back(static_cast<uint8_t>(group * cAlphabetGroupSize + index));
            }
        }
    }
    if (alphabet.empty()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block alphabet is empty."_el};
    }
    return alphabet;
}

auto Bzip2Decoder::readSelectors(const std::size_t groupCount) -> std::vector<uint8_t> {
    const auto selectorCount = _reader.readBits(15U);
    if (selectorCount == 0U || selectorCount > cMaximumSelectorCount) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 selector count is invalid."_el};
    }
    auto mtf = std::vector<uint8_t>(groupCount);
    std::iota(mtf.begin(), mtf.end(), uint8_t{});
    auto result = std::vector<uint8_t>{};
    result.reserve(selectorCount);
    for (auto selector = uint32_t{}; selector < selectorCount; ++selector) {
        auto index = std::size_t{};
        while (_reader.readBits(1U) != 0U) {
            if (++index >= groupCount) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 selector is invalid."_el};
            }
        }
        const auto value = mtf[index];
        mtf.erase(mtf.begin() + static_cast<std::ptrdiff_t>(index));
        mtf.insert(mtf.begin(), value);
        result.push_back(value);
    }
    return result;
}

[[nodiscard]] auto Bzip2Decoder::readTables(const std::size_t groupCount, const std::size_t alphaSize)
    -> std::vector<Bzip2HuffmanTable> {
    auto result = std::vector<Bzip2HuffmanTable>{};
    result.reserve(groupCount);
    for (auto group = std::size_t{}; group < groupCount; ++group) {
        auto current = static_cast<int>(_reader.readBits(5U));
        auto lengths = std::vector<uint8_t>{};
        lengths.reserve(alphaSize);
        for (auto symbol = std::size_t{}; symbol < alphaSize; ++symbol) {
            while (_reader.readBits(1U) != 0U) {
                current += _reader.readBits(1U) == 0U ? 1 : -1;
                if (current < 1 || current > cMaximumCodeLength) {
                    throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 code length is invalid."_el};
                }
            }
            lengths.push_back(static_cast<uint8_t>(current));
        }
        result.emplace_back(lengths);
    }
    return result;
}

[[nodiscard]] auto Bzip2Decoder::decodeSymbols(
    const std::vector<uint8_t> &alphabet,
    const std::vector<uint8_t> &selectors,
    const std::vector<Bzip2HuffmanTable> &tables) -> mem::ByteBlock {
    auto list = alphabet;
    auto result = mem::ByteBlockEditor{};
    result.reserve(unit::ByteLength::fromSizeT(_blockSize));
    auto selector = std::size_t{};
    auto remaining = std::size_t{};
    auto tableIndex = uint8_t{};
    auto nextSymbol = [&]() -> uint16_t {
        if (remaining == 0U) {
            if (selector >= selectors.size()) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 selectors are exhausted."_el};
            }
            tableIndex = selectors[selector++];
            remaining = cSelectorGroupSize;
        }
        --remaining;
        return tables[tableIndex].decode(_reader);
    };
    const auto endSymbol = static_cast<uint16_t>(alphabet.size() + 1U);
    auto symbol = nextSymbol();
    while (symbol != endSymbol) {
        if (symbol <= 1U) {
            auto run = std::size_t{};
            auto power = std::size_t{1U};
            do {
                if (run > _blockSize || power > _blockSize) {
                    throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 zero run is excessive."_el};
                }
                run += symbol == 0U ? power : power * 2U;
                power *= 2U;
                symbol = nextSymbol();
            } while (symbol <= 1U);
            if (run > _blockSize - result.length().toSizeT()) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block exceeds its limit."_el};
            }
            result.append(mem::Byte{list.front()}, unit::ByteLength::fromSizeT(run));
            if (symbol == endSymbol) {
                break;
            }
        }
        const auto index = static_cast<std::size_t>(symbol - 1U);
        if (index >= list.size()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 MTF symbol is invalid."_el};
        }
        const auto value = list[index];
        list.erase(list.begin() + static_cast<std::ptrdiff_t>(index));
        list.insert(list.begin(), value);
        if (result.length().toSizeT() >= _blockSize) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block exceeds its limit."_el};
        }
        result.append(mem::Byte{value});
        symbol = nextSymbol();
    }
    return result;
}

[[nodiscard]] auto Bzip2Decoder::inverseBwt(const mem::ConstByteSpan lastColumn, const std::size_t originalPointer)
    -> mem::ByteBlock {
    if (originalPointer >= lastColumn.size()) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 BWT pointer is invalid."_el};
    }
    auto counts = std::array<std::size_t, cAlphabetValueCount + 1U>{};
    for (const auto value : lastColumn) {
        ++counts[value.toUInt8() + 1U];
    }
    for (auto index = std::size_t{1U}; index < counts.size(); ++index) {
        counts[index] += counts[index - 1U];
    }
    auto next = std::vector<std::size_t>(lastColumn.size());
    // Prefix sums partition the rows, so each slot receives exactly one index into lastColumn.
    auto positions = counts;
    for (auto index = std::size_t{}; index < lastColumn.size(); ++index) {
        next[positions[lastColumn[index].toUInt8()]++] = index;
    }
    auto result = mem::ByteBlockEditor{};
    result.reserve(unit::ByteLength::fromSizeT(lastColumn.size()));
    auto row = originalPointer;
    for (auto index = std::size_t{}; index < lastColumn.size(); ++index) {
        row = next[row];
        result.append(lastColumn[row]);
    }
    return result;
}

void Bzip2Decoder::undoRle1(const mem::ConstByteSpan data, Bzip2Crc &crc) {
    auto position = std::size_t{};
    auto previous = uint8_t{};
    auto run = std::size_t{};
    while (position < data.size()) {
        const auto value = data[position++].toUInt8();
        append(value);
        crc.update(value);
        run = run != 0U && value == previous ? run + 1U : 1U;
        previous = value;
        if (run == cRleLiteralLimit) {
            if (position >= data.size()) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 RLE count is truncated."_el};
            }
            const auto extra = data[position++].toUInt8();
            append(value, extra);
            for (auto index = uint16_t{}; index < extra; ++index) {
                crc.update(value);
            }
            run = 0U;
        }
    }
}

void Bzip2Decoder::decodeBlock() {
    const auto expectedCrc = _reader.readBits(32U);
    if (_reader.readBits(1U) != 0U) {
        throw CompressionError{
            CompressionErrorReason::UnsupportedFeature, "Randomized bzip2 blocks are unsupported."_el};
    }
    const auto originalPointer = _reader.readBits(24U);
    const auto alphabet = readAlphabet();
    const auto groupCount = _reader.readBits(3U);
    if (groupCount < cMinimumGroupCount || groupCount > cMaximumGroupCount) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 Huffman group count is invalid."_el};
    }
    const auto selectors = readSelectors(groupCount);
    const auto tables = readTables(groupCount, alphabet.size() + 2U);
    const auto lastColumn = decodeSymbols(alphabet, selectors, tables);
    const auto transformed = inverseBwt(lastColumn.span(), originalPointer);
    auto crc = Bzip2Crc{};
    undoRle1(transformed.span(), crc);
    if (crc.value() != expectedCrc) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Bzip2 block CRC is invalid."_el};
    }
    _combinedCrc = std::rotl(_combinedCrc, 1) ^ static_cast<uint32_t>(expectedCrc);
}

}
