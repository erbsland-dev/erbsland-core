// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardHuffmanEncoder.hpp"

#include "ZstandardReverseBitWriter.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <utility>

namespace erbsland::compression::impl {

ZstandardHuffmanEncoder::ZstandardHuffmanEncoder(mem::ConstByteSpan literals) noexcept : _literals{literals} {
}

auto ZstandardHuffmanEncoder::encode() const -> std::optional<mem::ByteBuffer> {
    auto weights = std::array<uint8_t, 256U>{};
    auto codes = std::array<ZstandardHuffmanCode, 256U>{};
    auto tableBits = uint8_t{};
    const auto highestSymbol = buildTree(_literals, weights, codes, tableBits);
    if (!highestSymbol.has_value()) {
        return std::nullopt;
    }
    auto tree = mem::ByteBuffer{};
    tree.reserve(unit::ByteLength::fromSizeT(1U + (*highestSymbol + 1U) / 2U));
    tree.append(mem::Byte{static_cast<uint8_t>(127U + *highestSymbol)});
    for (auto symbol = std::size_t{}; symbol < *highestSymbol; symbol += 2U) {
        tree.append(mem::Byte{static_cast<uint8_t>((weights[symbol] << 4U) | weights[symbol + 1U])});
    }
    const auto fourStreams = _literals.size() >= 1024U;
    auto streams = mem::ByteBuffer{};
    if (fourStreams) {
        const auto streamRegeneratedSize = (_literals.size() + 3U) / 4U;
        const auto fourthSize = _literals.size() - streamRegeneratedSize * 3U;
        const auto stream1 = encodeStream(_literals, 0U, streamRegeneratedSize, codes);
        const auto stream2 = encodeStream(_literals, streamRegeneratedSize, streamRegeneratedSize, codes);
        const auto stream3 = encodeStream(_literals, streamRegeneratedSize * 2U, streamRegeneratedSize, codes);
        const auto stream4 = encodeStream(_literals, streamRegeneratedSize * 3U, fourthSize, codes);
        if (stream1.length().toRawValue() > 65'535U || stream2.length().toRawValue() > 65'535U ||
            stream3.length().toRawValue() > 65'535U) {
            return std::nullopt;
        }
        streams.reserve(
            unit::ByteLength{6U}
                .addedOrThrow(stream1.length())
                .addedOrThrow(stream2.length())
                .addedOrThrow(stream3.length())
                .addedOrThrow(stream4.length()));
        append16(streams, stream1.length().toSizeTOrThrow());
        append16(streams, stream2.length().toSizeTOrThrow());
        append16(streams, stream3.length().toSizeTOrThrow());
        for (const auto *stream : {&stream1, &stream2, &stream3, &stream4}) {
            streams.append(stream->span());
        }
    } else {
        streams = encodeStream(_literals, 0U, _literals.size(), codes);
    }
    const auto compressedSize = tree.length().addedOrThrow(streams.length()).toSizeTOrThrow();
    if (compressedSize > 0x3ffffU) {
        return std::nullopt;
    }
    auto output = mem::ByteBuffer{};
    output.reserve(unit::ByteLength::fromSizeT(5U + compressedSize));
    appendHeader(output, _literals.size(), compressedSize, fourStreams);
    output.append(tree.span());
    output.append(streams.span());
    return output;
}

auto ZstandardHuffmanEncoder::buildTree(
    mem::ConstByteSpan literals,
    std::array<uint8_t, 256U> &weights,
    std::array<ZstandardHuffmanCode, 256U> &codes,
    uint8_t &tableBits) -> std::optional<std::size_t> {
    auto frequencies = std::array<std::size_t, 256U>{};
    for (const auto literal : literals) {
        ++frequencies[literal.toUInt8()];
    }
    auto symbols = std::vector<uint16_t>{};
    for (auto symbol = std::size_t{}; symbol < frequencies.size(); ++symbol) {
        if (frequencies[symbol] != 0U) {
            symbols.push_back(static_cast<uint16_t>(symbol));
        }
    }
    if (symbols.size() < 2U || symbols.back() > 128U) {
        return std::nullopt;
    }
    std::ranges::sort(symbols, [&frequencies](const uint16_t left, const uint16_t right) -> bool {
        return frequencies[left] == frequencies[right] ? left < right : frequencies[left] > frequencies[right];
    });
    const auto lowerBits = static_cast<uint8_t>(std::bit_width(symbols.size()) - 1);
    const auto powerOfTwo = std::has_single_bit(symbols.size());
    tableBits = static_cast<uint8_t>(lowerBits + (powerOfTwo ? 0U : 1U));
    const auto shortCount = powerOfTwo ? std::size_t{} : (std::size_t{1U} << tableBits) - symbols.size();
    for (auto index = std::size_t{}; index < symbols.size(); ++index) {
        const auto length = static_cast<uint8_t>(tableBits - (index < shortCount ? 1U : 0U));
        weights[symbols[index]] = static_cast<uint8_t>(tableBits + 1U - length);
    }
    auto counts = std::array<uint32_t, 9U>{};
    for (const auto weight : weights) {
        if (weight != 0U) {
            ++counts[weight];
        }
    }
    auto next = uint32_t{};
    for (auto weight = std::size_t{1U}; weight <= tableBits; ++weight) {
        const auto current = next;
        next += counts[weight] << (weight - 1U);
        counts[weight] = current;
    }
    for (auto symbol = std::size_t{}; symbol < weights.size(); ++symbol) {
        const auto weight = weights[symbol];
        if (weight == 0U) {
            continue;
        }
        const auto length = static_cast<uint8_t>(tableBits + 1U - weight);
        codes[symbol] = ZstandardHuffmanCode{static_cast<uint16_t>(counts[weight] >> (tableBits - length)), length};
        counts[weight] += uint32_t{1U} << (weight - 1U);
    }
    return static_cast<std::size_t>(*std::ranges::max_element(symbols));
}

auto ZstandardHuffmanEncoder::encodeStream(
    mem::ConstByteSpan literals,
    const std::size_t begin,
    const std::size_t size,
    const std::array<ZstandardHuffmanCode, 256U> &codes) -> mem::ByteBuffer {
    auto writer = ZstandardReverseBitWriter{};
    for (auto position = begin; position < begin + size; ++position) {
        const auto &code = codes[literals[position].toUInt8()];
        writer.append(code.value, code.bitCount);
    }
    return writer.takeBytes();
}

void ZstandardHuffmanEncoder::appendHeader(
    mem::ByteBuffer &output,
    const std::size_t regeneratedSize,
    const std::size_t compressedSize,
    const bool fourStreams) {
    if (regeneratedSize < 1024U && compressedSize < 1024U) {
        const auto format = fourStreams ? uint8_t{1U} : uint8_t{0U};
        output.append(
            mem::Byte{static_cast<uint8_t>(
                std::size_t{2U} | (static_cast<std::size_t>(format) << 2U) | ((regeneratedSize & 0x0fU) << 4U))});
        output.append(mem::Byte{static_cast<uint8_t>((regeneratedSize >> 4U) | ((compressedSize & 3U) << 6U))});
        output.append(mem::Byte{static_cast<uint8_t>(compressedSize >> 2U)});
    } else if (regeneratedSize < 16'384U && compressedSize < 16'384U) {
        output.append(mem::Byte{static_cast<uint8_t>(2U | (2U << 2U) | ((regeneratedSize & 0x0fU) << 4U))});
        output.append(mem::Byte{static_cast<uint8_t>(regeneratedSize >> 4U)});
        output.append(
            mem::Byte{static_cast<uint8_t>(((regeneratedSize >> 12U) & 3U) | ((compressedSize & 0x3fU) << 2U))});
        output.append(mem::Byte{static_cast<uint8_t>(compressedSize >> 6U)});
    } else {
        output.append(mem::Byte{static_cast<uint8_t>(2U | (3U << 2U) | ((regeneratedSize & 0x0fU) << 4U))});
        output.append(mem::Byte{static_cast<uint8_t>(regeneratedSize >> 4U)});
        output.append(
            mem::Byte{static_cast<uint8_t>(((regeneratedSize >> 12U) & 0x3fU) | ((compressedSize & 3U) << 6U))});
        output.append(mem::Byte{static_cast<uint8_t>(compressedSize >> 2U)});
        output.append(mem::Byte{static_cast<uint8_t>(compressedSize >> 10U)});
    }
}

void ZstandardHuffmanEncoder::append16(mem::ByteBuffer &output, const std::size_t value) {
    output.append(mem::Byte{static_cast<uint8_t>(value)});
    output.append(mem::Byte{static_cast<uint8_t>(value >> 8U)});
}

}
