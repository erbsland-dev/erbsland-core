// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PunycodeCodec.hpp"

#include "../../../err/ParseError.hpp"
#include "../../StringCharReader.hpp"
#include "../../StringEditor.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace erbsland::text::punycode::impl {

constexpr auto cBase = uint64_t{36U};
constexpr auto cTMin = uint64_t{1U};
constexpr auto cTMax = uint64_t{26U};
constexpr auto cSkew = uint64_t{38U};
constexpr auto cDamp = uint64_t{700U};
constexpr auto cInitialBias = uint64_t{72U};
constexpr auto cInitialN = uint64_t{128U};
constexpr auto cDelimiter = U'-';
constexpr auto cMaximumCodePoint = uint64_t{0x10FFFFU};

auto threshold(const uint64_t index, const uint64_t bias) noexcept -> uint64_t {
    if (index <= bias + cTMin) {
        return cTMin;
    }
    if (index >= bias + cTMax) {
        return cTMax;
    }
    return index - bias;
}

auto adapt(uint64_t delta, const uint64_t pointCount, const bool firstTime) noexcept -> uint64_t {
    delta = firstTime ? delta / cDamp : delta / 2U;
    delta += delta / pointCount;
    auto index = uint64_t{};
    while (delta > ((cBase - cTMin) * cTMax) / 2U) {
        delta /= cBase - cTMin;
        index += cBase;
    }
    return index + (((cBase - cTMin + 1U) * delta) / (delta + cSkew));
}

auto encodeDigit(const uint64_t value) noexcept -> Char {
    return value < 26U ? Char{static_cast<char32_t>(U'a' + value)} : Char{static_cast<char32_t>(U'0' + value - 26U)};
}

auto decodeDigit(const Char character) noexcept -> std::optional<uint64_t> {
    const auto value = character.toRawValue();
    if (value >= U'a' && value <= U'z') {
        return static_cast<uint64_t>(value - U'a');
    }
    if (value >= U'A' && value <= U'Z') {
        return static_cast<uint64_t>(value - U'A');
    }
    if (value >= U'0' && value <= U'9') {
        return static_cast<uint64_t>(value - U'0') + 26U;
    }
    return {};
}

auto readCodePoints(const String &text) -> std::vector<char32_t> {
    if (!text.isValidUtf8()) {
        throw err::ParseError{"The Punycode input is not valid UTF-8."};
    }
    auto result = std::vector<char32_t>{};
    result.reserve(text.characterLength().toSizeT());
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isValidUnicode()) {
            throw err::ParseError{"The Punycode input contains an invalid Unicode character."};
        }
        result.push_back(character.toRawValue());
    }
    return result;
}

auto encodePunycodePayload(const String &text) -> String {
    const auto input = readCodePoints(text);
    auto output = StringEditor{};
    auto basicCount = uint64_t{};
    for (const auto codePoint : input) {
        if (codePoint < 0x80U) {
            output.append(Char{codePoint});
            ++basicCount;
        }
    }
    auto handledCount = basicCount;
    if (basicCount != 0U) {
        output.append(Char{cDelimiter});
    }
    auto n = cInitialN;
    auto delta = uint64_t{};
    auto bias = cInitialBias;
    const auto inputCount = static_cast<uint64_t>(input.size());
    while (handledCount < inputCount) {
        auto minimum = std::numeric_limits<uint64_t>::max();
        for (const auto codePoint : input) {
            const auto value = static_cast<uint64_t>(codePoint);
            if (value >= n && value < minimum) {
                minimum = value;
            }
        }
        if (minimum == std::numeric_limits<uint64_t>::max()) {
            throw err::ParseError{"The Punycode encoder could not select the next code point."};
        }
        const auto step = minimum - n;
        if (step != 0U && handledCount + 1U > (std::numeric_limits<uint64_t>::max() - delta) / step) {
            throw err::ParseError{"The Punycode input exceeds the arithmetic limits."};
        }
        delta += step * (handledCount + 1U);
        n = minimum;
        for (const auto codePoint : input) {
            const auto value = static_cast<uint64_t>(codePoint);
            if (value < n) {
                if (delta == std::numeric_limits<uint64_t>::max()) {
                    throw err::ParseError{"The Punycode input exceeds the arithmetic limits."};
                }
                ++delta;
            }
            if (value != n) {
                continue;
            }
            auto quotient = delta;
            for (auto index = cBase;; index += cBase) {
                const auto t = threshold(index, bias);
                if (quotient < t) {
                    break;
                }
                output.append(encodeDigit(t + ((quotient - t) % (cBase - t))));
                quotient = (quotient - t) / (cBase - t);
            }
            output.append(encodeDigit(quotient));
            bias = adapt(delta, handledCount + 1U, handledCount == basicCount);
            delta = 0U;
            ++handledCount;
        }
        if (delta == std::numeric_limits<uint64_t>::max() || n == cMaximumCodePoint) {
            throw err::ParseError{"The Punycode input exceeds the Unicode or arithmetic limits."};
        }
        ++delta;
        ++n;
    }
    return output;
}

auto decodePunycodePayload(const String &text) -> String {
    if (!text.isValidUtf8()) {
        throw err::ParseError{"The Punycode input is not valid UTF-8."};
    }
    auto encoded = std::vector<Char>{};
    encoded.reserve(text.characterLength().toSizeT());
    auto delimiterIndex = std::optional<std::size_t>{};
    auto reader = StringCharReader{text};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAscii()) {
            throw err::ParseError{"A Punycode payload must contain ASCII characters only."};
        }
        if (character == cDelimiter) {
            delimiterIndex = encoded.size();
        }
        encoded.push_back(character);
    }
    auto output = std::vector<char32_t>{};
    auto inputIndex = std::size_t{};
    if (delimiterIndex.has_value()) {
        for (; inputIndex < delimiterIndex.value(); ++inputIndex) {
            const auto character = encoded[inputIndex];
            if (!character.isAscii()) {
                throw err::ParseError{"The basic Punycode prefix is not ASCII."};
            }
            output.push_back(character.toRawValue());
        }
        ++inputIndex;
    }
    auto n = cInitialN;
    auto index = uint64_t{};
    auto bias = cInitialBias;
    while (inputIndex < encoded.size()) {
        const auto oldIndex = index;
        auto weight = uint64_t{1U};
        for (auto digitIndex = cBase;; digitIndex += cBase) {
            if (inputIndex >= encoded.size()) {
                throw err::ParseError{"The Punycode payload ends inside a variable-length integer."};
            }
            const auto digit = decodeDigit(encoded[inputIndex++]);
            if (!digit.has_value()) {
                throw err::ParseError{"The Punycode payload contains an invalid digit."};
            }
            if (digit.value() > (std::numeric_limits<uint64_t>::max() - index) / weight) {
                throw err::ParseError{"The Punycode payload exceeds the arithmetic limits."};
            }
            index += digit.value() * weight;
            const auto t = threshold(digitIndex, bias);
            if (digit.value() < t) {
                break;
            }
            if (weight > std::numeric_limits<uint64_t>::max() / (cBase - t)) {
                throw err::ParseError{"The Punycode payload exceeds the arithmetic limits."};
            }
            weight *= cBase - t;
        }
        const auto outputCount = static_cast<uint64_t>(output.size()) + 1U;
        bias = adapt(index - oldIndex, outputCount, oldIndex == 0U);
        const auto increment = index / outputCount;
        if (increment > cMaximumCodePoint - n) {
            throw err::ParseError{"The Punycode payload decodes outside Unicode."};
        }
        n += increment;
        index %= outputCount;
        if (n >= 0xD800U && n <= 0xDFFFU) {
            throw err::ParseError{"The Punycode payload decodes to a Unicode surrogate."};
        }
        output.insert(output.begin() + static_cast<std::ptrdiff_t>(index), static_cast<char32_t>(n));
        ++index;
    }
    auto result = StringEditor{};
    for (const auto codePoint : output) {
        result.append(Char{codePoint});
    }
    return result;
}

}
