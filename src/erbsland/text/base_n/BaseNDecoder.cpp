// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BaseNDecoder.hpp"

#include "../StringCharReader.hpp"

#include "../../err/Exception.hpp"
#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/impl/UnsafeByteBlockBuffer.hpp"

#include <cstdint>
#include <numeric>
#include <string_view>
#include <utility>

namespace erbsland::text::base_n {

BaseNDecoder::BaseNDecoder(AnyString text, BaseNFormat format) : _text{std::move(text)}, _format{std::move(format)} {
}

auto BaseNDecoder::toData(const unit::ByteLength maximum) const -> std::optional<mem::ByteBlock> {
    try {
        return toDataOrThrow(maximum);
    } catch (const err::Exception &) {
        return {};
    }
}

auto BaseNDecoder::toDataOrThrow(const unit::ByteLength maximum) const -> mem::ByteBlock {
    const auto throwAt = [](const std::string_view message, const unit::CpIndex position) -> void {
        throw err::ParseError{message, position};
    };
    auto reader = StringCharReader{_text};
    auto digitCount = std::size_t{0};
    auto paddingCount = std::size_t{0};
    auto seenPadding = false;
    while (!reader.isAtEnd()) {
        const auto position = reader.position();
        const auto character = reader.read();
        if (_format.whitespace().contains(character)) {
            continue;
        }
        if (_format.padding().has_value() && character == _format.padding().value()) {
            seenPadding = true;
            ++paddingCount;
            continue;
        }
        if (seenPadding) {
            throwAt("An alphabet character follows Base-N padding.", position);
        }
        if (!_format.valueFor(character).has_value()) {
            throwAt("The Base-N text contains a character outside the alphabet.", position);
        }
        ++digitCount;
    }

    const auto bitsPerCharacter = _format.bitsPerCharacter();
    const auto quantum = 8U / std::gcd<unsigned int>(8U, static_cast<unsigned int>(bitsPerCharacter));
    const auto remainder = digitCount % quantum;
    auto expectedPadding = std::size_t{0};
    if (remainder != 0U) {
        expectedPadding = quantum - remainder;
        const auto usefulBits = remainder * bitsPerCharacter;
        if (usefulBits < 8U || usefulBits % 8U >= bitsPerCharacter) {
            throwAt("The Base-N text has an invalid final group length.", reader.position());
        }
    }
    if (paddingCount != 0U && paddingCount != expectedPadding) {
        throwAt("The Base-N text has non-canonical padding.", reader.position());
    }
    if (_format.hasFlag(BaseNFormatFlag::RequirePadding) && paddingCount != expectedPadding) {
        throwAt("The Base-N text is missing required padding.", reader.position());
    }

    const auto outputSize = (digitCount / 8U) * bitsPerCharacter + ((digitCount % 8U) * bitsPerCharacter) / 8U;
    if (maximum.isFinite() && outputSize > maximum.toSizeTOrThrow()) {
        throw err::OutOfRangeError{"The decoded Base-N data exceeds the maximum byte length."};
    }
    auto output = mem::impl::UnsafeByteBlockBuffer{unit::ByteLength::fromSizeT(outputSize)};
    auto outputBytes = output.data();
    auto outputIndex = std::size_t{0};
    reader.reset();
    auto accumulator = uint32_t{0};
    auto availableBits = uint8_t{0};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (_format.whitespace().contains(character) ||
            (_format.padding().has_value() && character == _format.padding().value())) {
            continue;
        }
        accumulator = (accumulator << bitsPerCharacter) | _format.valueFor(character).value();
        availableBits = static_cast<uint8_t>(availableBits + bitsPerCharacter);
        if (availableBits >= 8U) {
            availableBits = static_cast<uint8_t>(availableBits - 8U);
            outputBytes[outputIndex++] = mem::Byte{static_cast<uint8_t>(accumulator >> availableBits)};
        }
        if (availableBits == 0U) {
            accumulator = 0U;
        } else {
            accumulator &= (1U << availableBits) - 1U;
        }
    }
    if (availableBits != 0U && accumulator != 0U) {
        throwAt("The Base-N text has non-zero unused trailing bits.", reader.position());
    }
    return output.take(unit::ByteLength::fromSizeT(outputIndex));
}

}
