// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bzip2Encoder.hpp"

#include "Bzip2Crc.hpp"

#include "../CodecBitOutput.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../mem/ByteBlockEditor.hpp"
#include "../../../text/Literals.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <limits>
#include <utility>
#include <vector>

namespace erbsland::compression::impl {

using namespace text::literals;

Bzip2Encoder::Bzip2Encoder(const CompressionLevel level) : _blockLevel{blockLevel(level)} {
}

[[nodiscard]] auto Bzip2Encoder::blockLevel(const CompressionLevel level) noexcept -> uint8_t {
    switch (level) {
    case CompressionLevel::Fastest:
        return 1U;
    case CompressionLevel::Fast:
        return 3U;
    case CompressionLevel::Default:
        return 6U;
    case CompressionLevel::High:
        return 8U;
    case CompressionLevel::Highest:
        return 9U;
    }
    return 6U;
}

[[nodiscard]] auto Bzip2Encoder::rle1(const mem::ConstByteSpan input) -> mem::ByteBlock {
    auto result = mem::ByteBlockEditor{};
    auto position = std::size_t{};
    while (position < input.size()) {
        auto run = std::size_t{1U};
        while (position + run < input.size() && input[position + run] == input[position] && run < cMaximumRleRun) {
            ++run;
        }
        const auto literalCount = std::min(run, cRleLiteralLimit);
        result.append(input[position], unit::ByteLength::fromSizeT(literalCount));
        if (run >= cRleLiteralLimit) {
            result.append(mem::Byte::fromCroppedUInt64(run - cRleLiteralLimit));
        }
        position += run;
    }
    return result;
}

[[nodiscard]] auto Bzip2Encoder::bwt(const mem::ConstByteSpan input) -> Bzip2BwtResult {
    const auto size = input.size();
    if (size > std::numeric_limits<uint32_t>::max()) {
        throw err::OutOfRangeError{"Bzip2 transform exceeds the supported block size."_el};
    }
    auto &order = _order;
    order.resize(size);
    auto &classes = _classes;
    classes.resize(size);
    auto counts = std::array<std::size_t, cAlphabetValueCount>{};
    for (const auto value : input) {
        ++counts[value.toUInt8()];
    }
    for (auto index = std::size_t{1U}; index < counts.size(); ++index) {
        counts[index] += counts[index - 1U];
    }
    for (auto index = size; index-- > 0U;) {
        order[--counts[input[index].toUInt8()]] = static_cast<uint32_t>(index);
    }
    classes[order[0U]] = 0U;
    auto classCount = uint32_t{1U};
    for (auto index = std::size_t{1U}; index < size; ++index) {
        if (input[order[index]] != input[order[index - 1U]]) {
            ++classCount;
        }
        classes[order[index]] = classCount - 1U;
    }
    auto &scratch = _scratch;
    scratch.resize(size);
    auto &classCounts = _classCounts;
    classCounts.resize(size);
    for (auto span = std::size_t{1U}; span < size; span *= 2U) {
        std::fill(classCounts.begin(), classCounts.begin() + classCount, uint32_t{});
        for (auto index = std::size_t{}; index < size; ++index) {
            const auto value = static_cast<std::size_t>(order[index]);
            const auto shifted = static_cast<uint32_t>(value >= span ? value - span : value + size - span);
            ++classCounts[classes[shifted]];
        }
        for (auto index = std::size_t{1U}; index < classCount; ++index) {
            classCounts[index] += classCounts[index - 1U];
        }
        for (auto index = size; index-- > 0U;) {
            const auto value = static_cast<std::size_t>(order[index]);
            const auto shifted = static_cast<uint32_t>(value >= span ? value - span : value + size - span);
            scratch[--classCounts[classes[shifted]]] = shifted;
        }
        order[scratch[0]] = 0U;
        auto newCount = uint32_t{1U};
        for (auto index = std::size_t{1U}; index < size; ++index) {
            const auto currentPosition = static_cast<std::size_t>(scratch[index]);
            const auto previousPosition = static_cast<std::size_t>(scratch[index - 1U]);
            const auto current = std::pair{classes[currentPosition], classes[(currentPosition + span) % size]};
            const auto previous = std::pair{classes[previousPosition], classes[(previousPosition + span) % size]};
            if (current != previous) {
                ++newCount;
            }
            order[currentPosition] = newCount - 1U;
        }
        order.swap(scratch);
        classes.swap(scratch);
        classCount = newCount;
        if (classCount == size || span > size / 2U) {
            break;
        }
    }
    auto result = Bzip2BwtResult{};
    auto lastColumn = mem::ByteBlockEditor{unit::ByteLength::fromSizeT(size)};
    for (auto index = std::size_t{}; index < size; ++index) {
        lastColumn.setOrThrow(unit::ByteIndex::fromSizeT(index), input[(order[index] + size - 1U) % size]);
        if (order[index] == 0U) {
            result.originalPointer = static_cast<uint32_t>(index);
        }
    }
    result.lastColumn = lastColumn;
    return result;
}

[[nodiscard]] auto Bzip2Encoder::mtfRle(const mem::ConstByteSpan input) -> Bzip2MtfResult {
    auto used = std::array<bool, cAlphabetValueCount>{};
    for (const auto value : input) {
        used[value.toUInt8()] = true;
    }
    auto result = Bzip2MtfResult{};
    for (auto value = std::size_t{}; value < used.size(); ++value) {
        if (used[value]) {
            result.alphabet.push_back(static_cast<uint8_t>(value));
        }
    }
    auto list = result.alphabet;
    auto zeroRun = std::size_t{};
    auto flushZeros = [&]() -> void {
        while (zeroRun != 0U) {
            --zeroRun;
            result.symbols.push_back((zeroRun & 1U) == 0U ? 0U : 1U);
            zeroRun >>= 1U;
        }
    };
    for (const auto byte : input) {
        const auto value = byte.toUInt8();
        const auto iterator = std::ranges::find(list, value);
        const auto position = static_cast<std::size_t>(iterator - list.begin());
        if (position == 0U) {
            ++zeroRun;
            continue;
        }
        flushZeros();
        result.symbols.push_back(static_cast<uint16_t>(position + 1U));
        list.erase(iterator);
        list.insert(list.begin(), value);
    }
    flushZeros();
    result.symbols.push_back(static_cast<uint16_t>(result.alphabet.size() + 1U));
    return result;
}

void Bzip2Encoder::encodeBlock(const mem::ConstByteSpan input) {
    auto crc = Bzip2Crc{};
    for (const auto value : input) {
        crc.update(value.toUInt8());
    }
    const auto blockCrc = crc.value();
    _combinedCrc = std::rotl(_combinedCrc, 1) ^ blockCrc;
    const auto transformed = rle1(input);
    const auto bwtResult = bwt(transformed.span());
    const auto mtfResult = mtfRle(bwtResult.lastColumn.span());
    _writer.writeBits(cBlockMarker, 48U);
    _writer.writeBits(blockCrc, 32U);
    _writer.writeBits(0U, 1U);
    _writer.writeBits(bwtResult.originalPointer, 24U);
    auto groups = std::array<bool, cAlphabetGroupCount>{};
    for (const auto value : mtfResult.alphabet) {
        groups[value / cAlphabetGroupSize] = true;
    }
    for (const auto value : groups) {
        _writer.writeBits(value ? 1U : 0U, 1U);
    }
    for (auto group = std::size_t{}; group < groups.size(); ++group) {
        if (!groups[group]) {
            continue;
        }
        for (auto index = std::size_t{}; index < cAlphabetGroupSize; ++index) {
            _writer.writeBits(
                std::ranges::find(mtfResult.alphabet, static_cast<uint8_t>(group * cAlphabetGroupSize + index)) !=
                        mtfResult.alphabet.end()
                    ? 1U
                    : 0U,
                1U);
        }
    }
    const auto selectorCount = (mtfResult.symbols.size() + cSelectorGroupSize - 1U) / cSelectorGroupSize;
    _writer.writeBits(cHuffmanGroupCount, 3U);
    _writer.writeBits(selectorCount, 15U);
    for (auto index = std::size_t{}; index < selectorCount; ++index) {
        _writer.writeBits(0U, 1U);
    }
    const auto alphaSize = mtfResult.alphabet.size() + 2U;
    auto codeLength = uint8_t{1U};
    while ((std::size_t{1U} << codeLength) < alphaSize) {
        ++codeLength;
    }
    for (auto group = std::size_t{}; group < cHuffmanGroupCount; ++group) {
        _writer.writeBits(codeLength, 5U);
        for (auto symbol = std::size_t{}; symbol < alphaSize; ++symbol) {
            _writer.writeBits(0U, 1U);
        }
    }
    for (const auto symbol : mtfResult.symbols) {
        _writer.writeBits(symbol, codeLength);
    }
}

void Bzip2Encoder::encodeStream(CodecReader &input, const CodecOutput::Write &output) {
    _writer.writeBits(cStreamMagic, 24U);
    _writer.writeBits(static_cast<uint8_t>('0') + _blockLevel, 8U);
    while (!input.atEnd()) {
        const auto block = input.block(static_cast<std::size_t>(_blockLevel) * cBlockInputFactor);
        encodeBlock(block.span());
        codecBitOutput::drain(_writer, output);
    }
    _writer.writeBits(cEndMarker, 48U);
    _writer.writeBits(_combinedCrc, 32U);
    codecBitOutput::drain(_writer, output, true);
}

}
