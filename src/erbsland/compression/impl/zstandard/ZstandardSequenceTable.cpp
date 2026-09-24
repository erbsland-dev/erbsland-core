// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardSequenceTable.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <array>

namespace erbsland::compression::impl {

using namespace text::literals;

void ZstandardSequenceTable::setPredefined(const ZstandardSequenceCode kind) {
    static constexpr auto literalCounts = std::array<int16_t, 36U>{
        4, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 2, 1, 1, 1, 1, 1, -1, -1, -1, -1};
    static constexpr auto offsetCounts = std::array<int16_t, 29U>{
        1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -1, -1, -1, -1, -1};
    static constexpr auto matchCounts = std::array<int16_t, 53U>{
        1,
        4,
        3,
        2,
        2,
        2,
        2,
        2,
        2,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1,
        -1};
    auto table = ZstandardFseTable{};
    if (kind == ZstandardSequenceCode::LiteralLength) {
        table.build(literalCounts, 6U);
    } else if (kind == ZstandardSequenceCode::Offset) {
        table.build(offsetCounts, 5U);
    } else {
        table.build(matchCounts, 6U);
    }
    convert(table, kind);
}

void ZstandardSequenceTable::setRle(const ZstandardSequenceCode kind, const uint8_t symbol) {
    auto table = ZstandardFseTable{};
    table.setRle(symbol);
    convert(table, kind);
}

void ZstandardSequenceTable::readCompressed(
    const mem::ConstByteSpan data, std::size_t &position, const ZstandardSequenceCode kind) {
    auto table = ZstandardFseTable{};
    if (kind == ZstandardSequenceCode::LiteralLength) {
        table.read(data, position, 35U, 9U);
    } else if (kind == ZstandardSequenceCode::Offset) {
        table.read(data, position, 31U, 8U);
    } else {
        table.read(data, position, 52U, 9U);
    }
    convert(table, kind);
}

auto ZstandardSequenceTable::entry(const uint32_t state) const -> const ZstandardSequenceEntry & {
    if (state >= _size) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard sequence state is out of range."_el};
    }
    return _entries[state];
}

void ZstandardSequenceTable::convert(const ZstandardFseTable &table, const ZstandardSequenceCode kind) {
    _accuracyLog = table.accuracyLog();
    _size = std::size_t{1U} << _accuracyLog;
    for (auto state = std::size_t{}; state < _size; ++state) {
        const auto &source = table.entry(static_cast<uint32_t>(state));
        auto target = valueFor(source.symbol, kind);
        target.stateBits = source.bitCount;
        target.stateBase = source.stateBase;
        _entries[state] = target;
    }
}

auto ZstandardSequenceTable::valueFor(const uint8_t symbol, const ZstandardSequenceCode kind)
    -> ZstandardSequenceEntry {
    static constexpr auto literalBases = std::array<uint32_t, 20U>{
        16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
    static constexpr auto literalBits =
        std::array<uint8_t, 20U>{1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    static constexpr auto matchBases = std::array<uint32_t, 21U>{
        35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515, 1027, 2051, 4099, 8195, 16387, 32771, 65539};
    static constexpr auto matchBits =
        std::array<uint8_t, 21U>{1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    auto result = ZstandardSequenceEntry{};
    result.symbol = symbol;
    if (kind == ZstandardSequenceCode::LiteralLength) {
        if (symbol < 16U) {
            result.baseline = symbol;
        } else if (symbol <= 35U) {
            result.baseline = literalBases[symbol - 16U];
            result.valueBits = literalBits[symbol - 16U];
        } else {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid literal-length code."_el};
        }
    } else if (kind == ZstandardSequenceCode::Offset) {
        if (symbol > 31U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid offset code."_el};
        }
        result.baseline = uint32_t{1U} << symbol;
        if (symbol >= 2U) {
            result.baseline -= 3U;
        }
        result.valueBits = symbol;
    } else if (symbol < 32U) {
        result.baseline = static_cast<uint32_t>(symbol) + 3U;
    } else if (symbol <= 52U) {
        result.baseline = matchBases[symbol - 32U];
        result.valueBits = matchBits[symbol - 32U];
    } else {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid match-length code."_el};
    }
    return result;
}

}
