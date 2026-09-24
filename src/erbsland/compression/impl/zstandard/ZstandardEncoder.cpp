// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardEncoder.hpp"

#include "ZstandardHuffmanEncoder.hpp"
#include "ZstandardReverseBitWriter.hpp"

#include "../../../err/LogicError.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

ZstandardEncoder::ZstandardEncoder(
    const mem::ConstByteSpan input, const CompressionLevel level, const std::size_t windowSize) :
    _input{input},
    _level{level},
    _windowSize{windowSize},
    _searchDepth{std::array<std::size_t, 5U>{1U, 4U, 16U, 64U, 256U}[static_cast<std::size_t>(level)]},
    _lazySteps{std::array<std::size_t, 5U>{0U, 0U, 1U, 2U, 4U}[static_cast<std::size_t>(level)]},
    _heads(65'536U, cNoPosition),
    _previous(input.size(), cNoPosition) {
    _previous.reserve(windowSize + 131072U);
}

void ZstandardEncoder::setInput(mem::ConstByteSpan input, std::size_t removed) {
    if (removed > _previous.size() || input.size() < _previous.size() - removed) {
        throw err::LogicError{"Zstandard input does not retain its indexed history."_el};
    }
    const auto retained = _previous.size() - removed;
    if (removed) {
        for (auto &position : _heads) {
            position =
                position == cNoPosition || position < removed ? cNoPosition : static_cast<uint32_t>(position - removed);
        }
        for (std::size_t i{}; i < retained; ++i) {
            const auto position = _previous[i + removed];
            _previous[i] =
                position == cNoPosition || position < removed ? cNoPosition : static_cast<uint32_t>(position - removed);
        }
    }
    _insertedUntil = retained > 3U ? retained - 3U : 0U;
    _input = input;
    _previous.resize(input.size(), cNoPosition);
}

auto ZstandardEncoder::encodeBlock(const std::size_t begin, const std::size_t size)
    -> std::optional<mem::ConstByteSpan> {
    if (begin > _input.size() || size > _input.size() - begin || size > 128U * 1024U) {
        throw err::LogicError{"Zstandard encoder block is out of range."_el};
    }
    const auto end = begin + size;
    auto &sequences = _sequences;
    sequences.clear();
    auto &literals = _literals;
    literals.clear();
    parseBlock(begin, end, sequences, literals);
    if (sequences.empty()) {
        return std::nullopt;
    }
    auto &output = _output;
    output.clear();
    output.reserve(unit::ByteLength::fromSizeT(size));
    appendLiterals(output, literals);
    appendSequenceCount(output, sequences.size());
    auto &literalSymbols = _literalSymbols;
    literalSymbols.clear();
    auto &offsetSymbols = _offsetSymbols;
    offsetSymbols.clear();
    auto &matchSymbols = _matchSymbols;
    matchSymbols.clear();
    literalSymbols.reserve(sequences.size());
    offsetSymbols.reserve(sequences.size());
    matchSymbols.reserve(sequences.size());
    for (auto &sequence : sequences) {
        encodeValues(sequence);
        literalSymbols.push_back(sequence.literalSymbol);
        offsetSymbols.push_back(sequence.offsetSymbol);
        matchSymbols.push_back(sequence.matchSymbol);
    }
    auto literalTable = ZstandardSequenceTable{};
    auto offsetTable = ZstandardSequenceTable{};
    auto matchTable = ZstandardSequenceTable{};
    auto literalDescription = std::optional<mem::Byte>{};
    auto offsetDescription = std::optional<mem::Byte>{};
    auto matchDescription = std::optional<mem::Byte>{};
    const auto literalMode =
        selectTable(ZstandardSequenceCode::LiteralLength, literalSymbols, literalTable, literalDescription);
    const auto offsetMode = selectTable(ZstandardSequenceCode::Offset, offsetSymbols, offsetTable, offsetDescription);
    const auto matchMode = selectTable(ZstandardSequenceCode::MatchLength, matchSymbols, matchTable, matchDescription);
    output.append(mem::Byte{static_cast<uint8_t>((literalMode << 6U) | (offsetMode << 4U) | (matchMode << 2U))});
    for (const auto description : {literalDescription, offsetDescription, matchDescription}) {
        if (description.has_value()) {
            output.append(*description);
        }
    }
    if (!buildStatePath(literalTable, literalSymbols, _literalStates) ||
        !buildStatePath(offsetTable, offsetSymbols, _offsetStates) ||
        !buildStatePath(matchTable, matchSymbols, _matchStates)) {
        return std::nullopt;
    }
    auto writer = ZstandardReverseBitWriter{};
    writer.append(_literalStates.front(), literalTable.accuracyLog());
    writer.append(_offsetStates.front(), offsetTable.accuracyLog());
    writer.append(_matchStates.front(), matchTable.accuracyLog());
    for (auto index = std::size_t{}; index < sequences.size(); ++index) {
        const auto &sequence = sequences[index];
        writer.append(sequence.offsetExtra, sequence.offsetBits);
        writer.append(sequence.matchExtra, sequence.matchBits);
        writer.append(sequence.literalExtra, sequence.literalBits);
        if (index + 1U < sequences.size()) {
            const auto &literalEntry = literalTable.entry(_literalStates[index]);
            const auto &matchEntry = matchTable.entry(_matchStates[index]);
            const auto &offsetEntry = offsetTable.entry(_offsetStates[index]);
            writer.append(_literalStates[index + 1U] - literalEntry.stateBase, literalEntry.stateBits);
            writer.append(_matchStates[index + 1U] - matchEntry.stateBase, matchEntry.stateBits);
            writer.append(_offsetStates[index + 1U] - offsetEntry.stateBase, offsetEntry.stateBits);
        }
    }
    const auto bitstream = writer.takeBytes();
    output.append(bitstream.span());
    return output.span();
}

auto ZstandardEncoder::findMatch(const std::size_t position, const std::size_t end) const noexcept -> ZstandardMatch {
    if (position + 4U > end) {
        return {};
    }
    auto candidate = _heads[hashAt(position)];
    auto best = ZstandardMatch{};
    for (auto depth = std::size_t{}; candidate != cNoPosition && depth < _searchDepth; ++depth) {
        if (candidate >= position) {
            candidate = _previous[candidate];
            continue;
        }
        const auto offset = position - candidate;
        if (offset > _windowSize) {
            break;
        }
        auto length = std::size_t{};
        while (position + length < end && _input[candidate + length] == _input[position + length]) {
            ++length;
        }
        if (length > best.length) {
            best = ZstandardMatch{candidate, length};
        }
        candidate = _previous[candidate];
    }
    return best.length >= 4U ? best : ZstandardMatch{};
}

void ZstandardEncoder::insertPosition(const std::size_t position) noexcept {
    if (position + 4U > _input.size()) {
        return;
    }
    const auto hash = hashAt(position);
    _previous[position] = _heads[hash];
    _heads[hash] = static_cast<uint32_t>(position);
}

void ZstandardEncoder::insertUntil(const std::size_t end) noexcept {
    while (_insertedUntil < end) {
        insertPosition(_insertedUntil++);
    }
}

auto ZstandardEncoder::hashAt(const std::size_t position) const noexcept -> std::size_t {
    auto value = uint32_t{};
    for (auto index = std::size_t{}; index < 4U; ++index) {
        value = (value << 8U) | _input[position + index].toUInt32();
    }
    return (value * 2'654'435'761U) >> 16U;
}

void ZstandardEncoder::parseBlock(
    const std::size_t begin,
    const std::size_t end,
    std::vector<ZstandardSequence> &sequences,
    mem::ByteBuffer &literals) {
    insertUntil(begin);
    auto position = begin;
    auto anchor = begin;
    while (position + 4U <= end) {
        auto matchPosition = position;
        auto match = findMatch(position, end);
        insertUntil(position + 1U);
        if (match.length >= 4U && _lazySteps != 0U) {
            for (auto step = std::size_t{1U}; step <= _lazySteps && position + step + 4U <= end; ++step) {
                const auto later = findMatch(position + step, end);
                if (later.length > match.length + step) {
                    match = later;
                    matchPosition = position + step;
                }
            }
        }
        if (match.length < 4U) {
            ++position;
            continue;
        }
        const auto literalLength = matchPosition - anchor;
        const auto offset = matchPosition - match.position;
        literals.append(_input.subspan(anchor, matchPosition - anchor));
        sequences.push_back(
            ZstandardSequence{
                static_cast<uint32_t>(literalLength),
                static_cast<uint32_t>(offset),
                static_cast<uint32_t>(match.length)});
        position = matchPosition + match.length;
        insertUntil(position);
        anchor = position;
    }
    literals.append(_input.subspan(anchor, end - anchor));
    insertUntil(end);
}

void ZstandardEncoder::encodeValues(ZstandardSequence &sequence) {
    static constexpr auto literalBases = std::array<uint32_t, 20U>{
        16, 18, 20, 22, 24, 28, 32, 40, 48, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};
    static constexpr auto literalBits =
        std::array<uint8_t, 20U>{1, 1, 1, 1, 2, 2, 3, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    static constexpr auto matchBases = std::array<uint32_t, 21U>{
        35, 37, 39, 41, 43, 47, 51, 59, 67, 83, 99, 131, 259, 515, 1027, 2051, 4099, 8195, 16387, 32771, 65539};
    static constexpr auto matchBits =
        std::array<uint8_t, 21U>{1, 1, 1, 1, 2, 2, 3, 3, 4, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    encodeLength(
        sequence.literalLength,
        15U,
        literalBases,
        literalBits,
        16U,
        sequence.literalSymbol,
        sequence.literalBits,
        sequence.literalExtra);
    const auto offsetValue = sequence.offset + 3U;
    sequence.offsetSymbol = static_cast<uint8_t>(std::bit_width(offsetValue) - 1);
    sequence.offsetBits = sequence.offsetSymbol;
    sequence.offsetExtra = sequence.offset - ((uint32_t{1U} << sequence.offsetSymbol) - 3U);
    encodeLength(
        sequence.matchLength,
        34U,
        matchBases,
        matchBits,
        32U,
        sequence.matchSymbol,
        sequence.matchBits,
        sequence.matchExtra);
}

void ZstandardEncoder::encodeLength(
    const uint32_t value,
    const uint32_t directMaximum,
    const std::span<const uint32_t> bases,
    const std::span<const uint8_t> bits,
    const uint8_t symbolOffset,
    uint8_t &symbol,
    uint8_t &extraBits,
    uint32_t &extra) {
    if (value <= directMaximum) {
        symbol = static_cast<uint8_t>(value - (directMaximum == 34U ? 3U : 0U));
        return;
    }
    auto index = bases.size() - 1U;
    while (index > 0U && value < bases[index]) {
        --index;
    }
    symbol = static_cast<uint8_t>(symbolOffset + index);
    extraBits = bits[index];
    extra = value - bases[index];
}

void ZstandardEncoder::appendLiterals(mem::ByteBuffer &output, const mem::ByteBuffer &literals) const {
    const auto size = literals.length().toSizeTOrThrow();
    const auto repeated = size > 1U &&
        std::ranges::all_of(
            literals.span(), [value = literals.get(unit::ByteIndex{})](const mem::Byte byte) { return byte == value; });
    const auto headerSize = size <= 31U ? std::size_t{1U} : (size <= 4095U ? std::size_t{2U} : std::size_t{3U});
    const auto rawSize = headerSize + (repeated ? 1U : size);
    if (_level != CompressionLevel::Fastest) {
        const auto compressed = ZstandardHuffmanEncoder{literals.span()}.encode();
        if (compressed.has_value() && compressed->length().toRawValue() < rawSize) {
            output.append(compressed->span());
            return;
        }
    }
    appendRawLiterals(output, literals, repeated);
}

void ZstandardEncoder::appendRawLiterals(
    mem::ByteBuffer &output, const mem::ByteBuffer &literals, const bool repeated) {
    const auto size = literals.length().toSizeTOrThrow();
    const auto type = repeated ? uint8_t{1U} : uint8_t{0U};
    if (size <= 31U) {
        output.append(mem::Byte{static_cast<uint8_t>((size << 3U) | type)});
    } else if (size <= 4095U) {
        output.append(mem::Byte{static_cast<uint8_t>(((size & 0x0fU) << 4U) | 0x04U | type)});
        output.append(mem::Byte{static_cast<uint8_t>(size >> 4U)});
    } else {
        output.append(mem::Byte{static_cast<uint8_t>(((size & 0x0fU) << 4U) | 0x0cU | type)});
        output.append(mem::Byte{static_cast<uint8_t>(size >> 4U)});
        output.append(mem::Byte{static_cast<uint8_t>(size >> 12U)});
    }
    if (repeated) {
        output.append(literals.get(unit::ByteIndex{}));
    } else {
        output.append(literals.span());
    }
}

void ZstandardEncoder::appendSequenceCount(mem::ByteBuffer &output, const std::size_t count) {
    if (count < 128U) {
        output.append(mem::Byte{static_cast<uint8_t>(count)});
    } else if (count < 0x7f00U) {
        output.append(mem::Byte{static_cast<uint8_t>(128U + (count >> 8U))});
        output.append(mem::Byte{static_cast<uint8_t>(count)});
    } else {
        const auto value = count - 0x7f00U;
        output.append(mem::Byte{uint8_t{255U}});
        output.append(mem::Byte{static_cast<uint8_t>(value)});
        output.append(mem::Byte{static_cast<uint8_t>(value >> 8U)});
    }
}

auto ZstandardEncoder::selectTable(
    const ZstandardSequenceCode kind,
    const std::vector<uint8_t> &symbols,
    ZstandardSequenceTable &table,
    std::optional<mem::Byte> &description) -> uint8_t {
    if (std::ranges::all_of(symbols, [value = symbols.front()](const uint8_t symbol) { return symbol == value; })) {
        table.setRle(kind, symbols.front());
        description = mem::Byte{symbols.front()};
        return 1U;
    }
    table.setPredefined(kind);
    return 0U;
}

auto ZstandardEncoder::buildStatePath(
    const ZstandardSequenceTable &table, const std::vector<uint8_t> &symbols, std::vector<uint32_t> &states) -> bool {
    states.resize(symbols.size());
    for (auto state = std::size_t{}; state < table.stateCount(); ++state) {
        if (table.entry(static_cast<uint32_t>(state)).symbol == symbols.back()) {
            states.back() = static_cast<uint32_t>(state);
            break;
        }
        if (state + 1U == table.stateCount()) {
            return false;
        }
    }
    for (auto index = symbols.size() - 1U; index > 0U; --index) {
        const auto target = states[index];
        auto found = false;
        for (auto state = std::size_t{}; state < table.stateCount(); ++state) {
            const auto &entry = table.entry(static_cast<uint32_t>(state));
            const auto stateLimit = static_cast<uint32_t>(entry.stateBase) + (uint32_t{1U} << entry.stateBits);
            if (entry.symbol == symbols[index - 1U] && target >= entry.stateBase && target < stateLimit) {
                states[index - 1U] = static_cast<uint32_t>(state);
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

}
