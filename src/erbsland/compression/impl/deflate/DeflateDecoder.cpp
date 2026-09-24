// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DeflateDecoder.hpp"

#include "DeflateTables.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

DeflateDecoder::DeflateDecoder(CodecReader &input, const DecompressionOptions &options, CodecOutput::Write output) :
    _reader{input, mem::BitOrder::LeastSignificantFirst},
    _options{options},
    _output{std::move(output), 32768U, options.maximumOutputLength()} {
}

void DeflateDecoder::decode() {
    auto isFinal = false;
    while (!isFinal) {
        isFinal = _reader.readBits(1U) != 0U;
        const auto type = _reader.readBits(2U);
        if (type == 0U) {
            decodeStored();
        } else if (type == 1U) {
            decodeCompressed(fixedLiteralTable(), fixedDistanceTable());
        } else if (type == 2U) {
            decodeDynamic();
        } else {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate block type is reserved."_el};
        }
    }
    _reader.requireEnd();
    validateExpectedLength();
    _output.flush();
}

auto DeflateDecoder::fixedLiteralTable() -> const DeflateHuffmanTable & {
    static const auto table = DeflateHuffmanTable{fixedLiteralLengths()};
    return table;
}

auto DeflateDecoder::fixedDistanceTable() -> const DeflateHuffmanTable & {
    static const auto table = DeflateHuffmanTable{std::vector<uint8_t>(cFixedDistanceCount, 5U)};
    return table;
}

[[nodiscard]] auto DeflateDecoder::fixedLiteralLengths() -> std::vector<uint8_t> {
    auto result = std::vector<uint8_t>(cFixedLiteralCount, 8U);
    std::fill(result.begin() + 144, result.begin() + 256, uint8_t{9U});
    std::fill(result.begin() + 256, result.begin() + 280, uint8_t{7U});
    std::fill(result.begin() + 280, result.end(), uint8_t{8U});
    return result;
}

void DeflateDecoder::decodeStored() {
    _reader.alignToByte();
    if (!_reader.canReadBytes(4U)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate stored header is truncated."_el};
    }
    const auto length = static_cast<uint16_t>(
        _reader.readByte().toUInt16() | static_cast<uint16_t>(_reader.readByte().toUInt16() << 8U));
    const auto inverse = static_cast<uint16_t>(
        _reader.readByte().toUInt16() | static_cast<uint16_t>(_reader.readByte().toUInt16() << 8U));
    if (static_cast<uint16_t>(length ^ 0xffffU) != inverse) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate stored length check failed."_el};
    }
    if (!_reader.canReadBytes(length)) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate stored payload is truncated."_el};
    }
    if (_output.length().addedOrThrow(unit::ByteLength{length}) > _options.maximumOutputLength()) {
        throw err::OutOfRangeError{"Deflate output exceeds the configured maximum."_el};
    }
    auto remaining = static_cast<std::size_t>(length);
    while (remaining != 0U) {
        const auto bytes = _reader.readBytes(remaining);
        _output.append(bytes.span());
        remaining -= bytes.length().toSizeT();
    }
}

void DeflateDecoder::decodeDynamic() {
    const auto literalCount = static_cast<std::size_t>(_reader.readBits(5U)) + 257U;
    const auto distanceCount = static_cast<std::size_t>(_reader.readBits(5U)) + 1U;
    const auto codeCount = static_cast<std::size_t>(_reader.readBits(4U)) + 4U;
    static constexpr auto cOrder = std::array<uint8_t, cCodeLengthCount>{
        16U, 17U, 18U, 0U, 8U, 7U, 9U, 6U, 10U, 5U, 11U, 4U, 12U, 3U, 13U, 2U, 14U, 1U, 15U};
    auto codeLengths = std::vector<uint8_t>(cCodeLengthCount);
    for (auto index = uint32_t{}; index < codeCount; ++index) {
        codeLengths[cOrder[index]] = static_cast<uint8_t>(_reader.readBits(3U));
    }
    const auto codeTable = DeflateHuffmanTable{codeLengths};
    auto lengths = std::vector<uint8_t>{};
    lengths.reserve(literalCount + distanceCount);
    while (lengths.size() < literalCount + distanceCount) {
        const auto symbol = codeTable.decode(_reader);
        if (symbol <= 15U) {
            lengths.push_back(static_cast<uint8_t>(symbol));
            continue;
        }
        uint8_t value{};
        uint32_t repetitions{};
        if (symbol == 16U) {
            if (lengths.empty()) {
                throw CompressionError{CompressionErrorReason::MalformedData, "Deflate repeat has no predecessor."_el};
            }
            value = lengths.back();
            repetitions = static_cast<uint32_t>(_reader.readBits(2U)) + 3U;
        } else if (symbol == 17U) {
            repetitions = static_cast<uint32_t>(_reader.readBits(3U)) + 3U;
        } else if (symbol == 18U) {
            repetitions = static_cast<uint32_t>(_reader.readBits(7U)) + 11U;
        } else {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate code-length symbol is invalid."_el};
        }
        if (repetitions > literalCount + distanceCount - lengths.size()) {
            throw CompressionError{
                CompressionErrorReason::MalformedData, "Deflate code-length repeat is excessive."_el};
        }
        lengths.insert(lengths.end(), repetitions, value);
    }
    const auto literalEnd = lengths.begin() + static_cast<std::ptrdiff_t>(literalCount);
    auto literalLengths = std::vector<uint8_t>(lengths.begin(), literalEnd);
    auto distanceLengths = std::vector<uint8_t>(literalEnd, lengths.end());
    if (literalLengths.size() <= cEndOfBlockSymbol || literalLengths[cEndOfBlockSymbol] == 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Deflate tree has no end-of-block code."_el};
    }
    if (std::ranges::all_of(distanceLengths, [](const uint8_t length) -> bool { return length == 0U; })) {
        distanceLengths[0] = 1U;
    }
    decodeCompressed(DeflateHuffmanTable{literalLengths}, DeflateHuffmanTable{distanceLengths});
}

void DeflateDecoder::decodeCompressed(
    const DeflateHuffmanTable &literalTable, const DeflateHuffmanTable &distanceTable) {
    while (true) {
        const auto symbol = literalTable.decode(_reader);
        if (symbol < cEndOfBlockSymbol) {
            _output.append(mem::Byte{static_cast<uint8_t>(symbol)});
            continue;
        }
        if (symbol == cEndOfBlockSymbol) {
            return;
        }
        if (symbol < cFirstLengthSymbol || symbol > cLastLengthSymbol) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate length symbol is invalid."_el};
        }
        const auto lengthIndex = static_cast<std::size_t>(symbol - cFirstLengthSymbol);
        const auto length = unit::ByteLength{
            static_cast<uint64_t>(deflate_tables::cLengthBase[lengthIndex]) +
            _reader.readBits(deflate_tables::cLengthExtra[lengthIndex])};
        const auto distanceSymbol = distanceTable.decode(_reader);
        if (distanceSymbol > cMaximumDistanceSymbol) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate distance symbol is invalid."_el};
        }
        const auto distance = unit::ByteLength{
            static_cast<uint64_t>(deflate_tables::cDistanceBase[distanceSymbol]) +
            _reader.readBits(deflate_tables::cDistanceExtra[distanceSymbol])};
        if (distance.isZero() || distance > _output.length()) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Deflate distance exceeds prior output."_el};
        }
        _output.appendRepeated(unit::ByteRange{unit::ByteIndex::end(_output.length() - distance), distance}, length);
    }
}

void DeflateDecoder::validateExpectedLength() const {
    if (_options.expectedOutputLength().has_value() && _output.length() != *_options.expectedOutputLength()) {
        throw CompressionError{CompressionErrorReason::LengthMismatch, "Deflate output length does not match."_el};
    }
}

}
