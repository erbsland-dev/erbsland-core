// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BaseNEncoder.hpp"

#include "../AnyString.hpp"
#include "../AnyStringBuilder.hpp"
#include "../String.hpp"
#include "../u16/U16String.hpp"
#include "../u32/U32String.hpp"
#include "../u8/U8String.hpp"

#include "../../err/OutOfRangeError.hpp"

#include <cstdint>
#include <limits>
#include <numeric>
#include <utility>

namespace erbsland::text::base_n {

BaseNEncoder::BaseNEncoder(mem::ByteBlock data, BaseNFormat format) :
    _data{std::move(data)}, _format{std::move(format)} {
}

auto BaseNEncoder::toString() const -> String {
    return build(StringKind::U8).toString();
}

auto BaseNEncoder::toU8String() const -> U8String {
    return build(StringKind::U8).toU8String();
}

auto BaseNEncoder::toU16String() const -> U16String {
    return build(StringKind::U16).toU16String();
}

auto BaseNEncoder::toU32String() const -> U32String {
    return build(StringKind::U32).toU32String();
}

auto BaseNEncoder::build(const StringKind kind) const -> AnyString {
    const auto bitsPerCharacter = _format.bitsPerCharacter();
    const auto byteCount = _data.length().toSizeT();
    const auto maximumSize = std::numeric_limits<std::size_t>::max();
    if (byteCount > maximumSize / 8U) {
        throw err::OutOfRangeError{"The Base-N output size cannot be represented."};
    }
    const auto bitCount = byteCount * 8U;
    const auto symbolCount = bitCount / bitsPerCharacter + (bitCount % bitsPerCharacter != 0U ? 1U : 0U);
    const auto quantum = 8U / std::gcd<unsigned int>(8U, static_cast<unsigned int>(bitsPerCharacter));
    auto paddedCount = symbolCount;
    if (_format.hasFlag(BaseNFormatFlag::EmitPadding) && symbolCount % quantum != 0U) {
        const auto paddingCount = quantum - (symbolCount % quantum);
        if (symbolCount > maximumSize - paddingCount) {
            throw err::OutOfRangeError{"The Base-N output size cannot be represented."};
        }
        paddedCount += paddingCount;
    }
    auto outputCharacterCount = paddedCount;
    if (_format.hasFlag(BaseNFormatFlag::WrapLines) && paddedCount != 0U) {
        const auto lineBreakCount = (paddedCount - 1U) / _format.lineLength().toSizeTOrThrow();
        const auto separatorLength = _format.lineSeparator().length().toSizeT();
        if (separatorLength != 0U && lineBreakCount > (maximumSize - outputCharacterCount) / separatorLength) {
            throw err::OutOfRangeError{"The wrapped Base-N output size cannot be represented."};
        }
        outputCharacterCount += lineBreakCount * separatorLength;
    }
    auto builder = AnyStringBuilder::withCapacity(kind, unit::CpLength::fromSizeTOrThrow(outputCharacterCount));
    auto linePosition = std::size_t{0};
    const auto appendCharacter = [&](const Char character) -> void {
        if (_format.hasFlag(BaseNFormatFlag::WrapLines) && linePosition == _format.lineLength().toSizeTOrThrow()) {
            builder.append(_format.lineSeparator());
            linePosition = 0;
        }
        builder.append(character);
        ++linePosition;
    };

    auto accumulator = uint32_t{0};
    auto availableBits = uint8_t{0};
    const auto mask = static_cast<uint32_t>((1U << bitsPerCharacter) - 1U);
    for (const auto byte : _data.span()) {
        accumulator = (accumulator << 8U) | byte.toUInt8();
        availableBits = static_cast<uint8_t>(availableBits + 8U);
        while (availableBits >= bitsPerCharacter) {
            availableBits = static_cast<uint8_t>(availableBits - bitsPerCharacter);
            appendCharacter(_format.characterFor(static_cast<uint8_t>((accumulator >> availableBits) & mask)));
        }
        if (availableBits == 0U) {
            accumulator = 0U;
        } else {
            accumulator &= (1U << availableBits) - 1U;
        }
    }
    if (availableBits != 0U) {
        appendCharacter(
            _format.characterFor(static_cast<uint8_t>((accumulator << (bitsPerCharacter - availableBits)) & mask)));
    }
    if (_format.hasFlag(BaseNFormatFlag::EmitPadding)) {
        for (std::size_t i = symbolCount; i < paddedCount; ++i) {
            appendCharacter(_format.padding().value());
        }
    }
    return builder.takeAnyString();
}

}
