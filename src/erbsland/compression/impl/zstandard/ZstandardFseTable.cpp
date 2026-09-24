// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardFseTable.hpp"

#include "ZstandardForwardBitReader.hpp"

#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <array>
#include <bit>

namespace erbsland::compression::impl {

using namespace text::literals;

void ZstandardFseTable::read(
    const mem::ConstByteSpan data,
    std::size_t &position,
    const uint8_t maximumSymbol,
    const uint8_t maximumAccuracyLog) {
    auto reader = ZstandardForwardBitReader{data, position};
    const auto accuracyLog = static_cast<uint8_t>(reader.read(4U) + 5U);
    if (accuracyLog > maximumAccuracyLog || accuracyLog > 9U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard FSE accuracy log is too large."_el};
    }
    auto remaining = static_cast<int32_t>((1U << accuracyLog) + 1U);
    auto threshold = static_cast<int32_t>(1U << accuracyLog);
    auto bitsNeeded = static_cast<uint8_t>(accuracyLog + 1U);
    auto symbol = uint16_t{};
    auto previousWasZero = false;
    auto counts = std::array<int16_t, 256U>{};
    while (remaining > 1 && symbol <= maximumSymbol) {
        if (previousWasZero) {
            auto end = symbol;
            while (reader.peek(12U) == 0xfffU) {
                const auto zeroRunMarker = reader.read(12U);
                if (zeroRunMarker != 0xfffU) {
                    throw CompressionError{
                        CompressionErrorReason::MalformedData, "Invalid Zstandard FSE zero-run marker."_el};
                }
                end = static_cast<uint16_t>(end + 18U);
                if (end > maximumSymbol) {
                    throw CompressionError{
                        CompressionErrorReason::MalformedData, "Zstandard FSE zero run exceeds the symbol range."_el};
                }
            }
            while (reader.peek(2U) == 3U) {
                const auto zeroRunMarker = reader.read(2U);
                if (zeroRunMarker != 3U) {
                    throw CompressionError{
                        CompressionErrorReason::MalformedData, "Invalid Zstandard FSE zero-run marker."_el};
                }
                end = static_cast<uint16_t>(end + 3U);
                if (end > maximumSymbol) {
                    throw CompressionError{
                        CompressionErrorReason::MalformedData, "Zstandard FSE zero run exceeds the symbol range."_el};
                }
            }
            end = static_cast<uint16_t>(end + reader.read(2U));
            if (end > maximumSymbol) {
                throw CompressionError{
                    CompressionErrorReason::MalformedData, "Zstandard FSE zero run exceeds the symbol range."_el};
            }
            symbol = end;
            previousWasZero = false;
            continue;
        }
        const auto maximum = 2 * threshold - 1 - remaining;
        const auto low = static_cast<int32_t>(reader.peek(bitsNeeded - 1U));
        int32_t count;
        if (low < maximum) {
            count = static_cast<int32_t>(reader.read(bitsNeeded - 1U));
        } else {
            count = static_cast<int32_t>(reader.read(bitsNeeded));
            if (count >= threshold) {
                count -= maximum;
            }
        }
        --count;
        const auto probability = count >= 0 ? count : 1;
        if (probability >= remaining) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard FSE normalized count."_el};
        }
        remaining -= probability;
        counts[symbol++] = static_cast<int16_t>(count);
        previousWasZero = count == 0;
        while (remaining < threshold) {
            --bitsNeeded;
            threshold >>= 1;
        }
    }
    if (remaining != 1) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard FSE normalized counts."_el};
    }
    position = reader.consumedBytePosition();
    build(std::span<const int16_t>{counts.data(), static_cast<std::size_t>(maximumSymbol) + 1U}, accuracyLog);
}

void ZstandardFseTable::build(const std::span<const int16_t> normalizedCounts, const uint8_t accuracyLog) {
    if (accuracyLog < 5U || accuracyLog > 9U || normalizedCounts.empty() || normalizedCounts.size() > 256U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard FSE table dimensions."_el};
    }
    const auto tableSize = std::size_t{1U} << accuracyLog;
    // Validate the complete distribution before any count controls an index or a shift.
    auto total = std::size_t{};
    for (const auto count : normalizedCounts) {
        if (count < -1) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard FSE probability."_el};
        }
        const auto probability = count == -1 ? std::size_t{1U} : static_cast<std::size_t>(count);
        if (probability > tableSize - total) {
            throw CompressionError{
                CompressionErrorReason::MalformedData, "Zstandard FSE probabilities exceed the table."_el};
        }
        total += probability;
    }
    if (total != tableSize) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard FSE probabilities do not sum."_el};
    }
    auto lowProbabilityStart = tableSize;
    auto next = std::array<uint16_t, 256U>{};
    for (auto symbol = std::size_t{}; symbol < normalizedCounts.size(); ++symbol) {
        const auto count = normalizedCounts[symbol];
        if (count == -1) {
            _entries[--lowProbabilityStart].symbol = static_cast<uint8_t>(symbol);
            next[symbol] = 1U;
        } else {
            next[symbol] = static_cast<uint16_t>(count);
        }
    }
    auto position = std::size_t{};
    const auto step = (tableSize >> 1U) + (tableSize >> 3U) + 3U;
    const auto mask = tableSize - 1U;
    for (auto symbol = std::size_t{}; symbol < normalizedCounts.size(); ++symbol) {
        for (auto index = int16_t{}; index < normalizedCounts[symbol]; ++index) {
            _entries[position].symbol = static_cast<uint8_t>(symbol);
            position = (position + step) & mask;
            while (position >= lowProbabilityStart) {
                position = (position + step) & mask;
            }
        }
    }
    if (position != 0U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard FSE probabilities do not sum."_el};
    }
    for (auto state = std::size_t{}; state < tableSize; ++state) {
        auto &entry = _entries[state];
        const auto nextState = next[entry.symbol]++;
        if (nextState == 0U) {
            throw CompressionError{CompressionErrorReason::MalformedData, "Invalid Zstandard FSE state."_el};
        }
        const auto highBit = static_cast<uint8_t>(std::bit_width(nextState) - 1);
        const auto bitCount = static_cast<uint8_t>(accuracyLog - highBit);
        entry.bitCount = bitCount;
        entry.stateBase =
            static_cast<uint16_t>((static_cast<uint32_t>(nextState) << bitCount) - static_cast<uint32_t>(tableSize));
    }
    _size = tableSize;
    _accuracyLog = accuracyLog;
}

void ZstandardFseTable::setRle(const uint8_t symbol) noexcept {
    _entries[0] = ZstandardFseEntry{symbol, 0U, 0U};
    _size = 1U;
    _accuracyLog = 0U;
}

auto ZstandardFseTable::entry(const uint32_t state) const -> const ZstandardFseEntry & {
    if (state >= _size) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Zstandard FSE state is out of range."_el};
    }
    return _entries[state];
}

}
