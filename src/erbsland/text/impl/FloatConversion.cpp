// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "FloatConversion.hpp"

#include "ThrowHelper.hpp"

#include "../StringConverter.hpp"
#include "../StringEditor.hpp"
#include "../u8/U8StringEditor.hpp"

#include <cerrno>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <format>
#include <limits>
#include <utility>

namespace erbsland::text::impl {

[[nodiscard]] auto presentationType(const FloatFormat &format) noexcept -> char {
    const auto uppercase = format.letterCase() == LetterCase::Uppercase;
    switch (format.style()) {
    case FloatFormat::Style::Fixed:
        return uppercase ? 'F' : 'f';
    case FloatFormat::Style::Scientific:
        return uppercase ? 'E' : 'e';
    case FloatFormat::Style::General:
        return uppercase ? 'G' : 'g';
    case FloatFormat::Style::Hexadecimal:
        return uppercase ? 'A' : 'a';
    default:
        return '\0';
    }
}

[[nodiscard]] auto formatPattern(const FloatFormat &format) -> std::string {
    const auto type = presentationType(format);
    if (!format.hasPrecision()) {
        if (type == '\0') {
            return "{}";
        }
        return StringConverter{
            String::fromJoined({String{"{:"}, String::fromCharacter(Char{static_cast<char32_t>(type)}), String{"}"}})}
            .toStdString();
    }
    const auto precision = String::fromInteger(format.precision().toRawValue());
    if (type == '\0') {
        return StringConverter{String::fromJoined({String{"{:."}, precision, String{"}"}})}.toStdString();
    }
    return StringConverter{
        String::fromJoined(
            {String{"{:."}, precision, String::fromCharacter(Char{static_cast<char32_t>(type)}), String{"}"}})}
        .toStdString();
}

auto formatFloat(const double value, const FloatFormat &format) -> String {
    return String{std::vformat(formatPattern(format), std::make_format_args(value))};
}

auto charsFormat(const FloatParseOptions::Style style) noexcept -> std::chars_format {
    switch (style) {
    case FloatParseOptions::Style::Fixed:
        return std::chars_format::fixed;
    case FloatParseOptions::Style::Scientific:
        return std::chars_format::scientific;
    case FloatParseOptions::Style::Hexadecimal:
        return std::chars_format::hex;
    default:
        return std::chars_format::general;
    }
}

auto floatParseSuccess(const double value) noexcept -> FloatParseResult {
    return FloatParseResult{
        .value = value,
        .status = FloatParseStatus::Success,
        .message = {},
    };
}

auto floatParseFailure(const FloatParseStatus status, const std::string_view message) noexcept -> FloatParseResult {
    return FloatParseResult{
        .value = {},
        .status = status,
        .message = message,
    };
}

auto readFloatText(StringCharReader reader) -> FloatTextResult {
    auto text = StringEditor{};
    while (true) {
        const auto next = reader.read();
        if (next.isEndOfData()) {
            break;
        }
        if (next.isSignal()) {
            return FloatTextResult{
                .text = {},
                .status = FloatParseStatus::ParseError,
                .message = "Floating point text contains an invalid character",
            };
        }
        text.append(next);
    }
    return FloatTextResult{
        .text = StringConverter{text}.toStdString(),
        .status = FloatParseStatus::Success,
        .message = {},
    };
}

#if defined(_LIBCPP_VERSION)
auto hasHexFloatPrefix(const std::string_view text) noexcept -> bool {
    auto index = std::size_t{0};
    if (!text.empty() && text.front() == '-') {
        index = 1;
    }
    return text.size() >= index + 2 && text[index] == '0' && (text[index + 1] == 'x' || text[index + 1] == 'X');
}

auto addHexFloatPrefix(std::string text) -> std::string {
    const auto insertOffset = !text.empty() && text.front() == '-' ? std::size_t{1} : std::size_t{0};
    text.insert(insertOffset, "0x");
    return text;
}

auto removeAddedHexPrefixFromLength(const std::size_t parsedLength, const bool hadMinus) noexcept -> std::size_t {
    const auto prefixOffset = hadMinus ? std::size_t{1} : std::size_t{0};
    if (parsedLength <= prefixOffset + 2) {
        return prefixOffset;
    }
    return parsedLength - 2;
}

auto parseDoubleWithStrtodText(std::string_view text) -> FloatParserResult {
    auto parseText = std::string{text};
    const auto *begin = parseText.c_str();
    char *end = nullptr;
    errno = 0;
    const auto value = std::strtod(begin, &end);
    if (end == begin) {
        return FloatParserResult{.value = {}, .parsedLength = 0U, .error = std::errc::invalid_argument};
    }
    return FloatParserResult{
        .value = value,
        .parsedLength = static_cast<std::size_t>(end - begin),
        .error = errno == ERANGE ? std::errc::result_out_of_range : std::errc{},
    };
}

auto parseDoubleWithStrtod(std::string_view text, const FloatParseOptions &options) -> FloatParserResult {
    auto parseText = std::string{text};
    const auto addedHexPrefix = options.style() == FloatParseOptions::Style::Hexadecimal && !hasHexFloatPrefix(text);
    if (addedHexPrefix) {
        parseText = addHexFloatPrefix(std::move(parseText));
    }

    auto result = parseDoubleWithStrtodText(parseText);
    if (result.error == std::errc::invalid_argument) {
        return result;
    }
    if (addedHexPrefix) {
        result.parsedLength = removeAddedHexPrefixFromLength(result.parsedLength, !text.empty() && text.front() == '-');
    }
    if (options.style() == FloatParseOptions::Style::Fixed) {
        const auto parsedText = text.substr(0U, result.parsedLength);
        const auto exponentOffset = parsedText.find_first_of("eEpP");
        if (exponentOffset != std::string_view::npos) {
            return parseDoubleWithStrtodText(text.substr(0U, exponentOffset));
        }
    }
    if (result.error == std::errc::result_out_of_range) {
        return FloatParserResult{
            .value = result.value,
            .parsedLength = result.parsedLength,
            .error = std::errc::result_out_of_range,
        };
    }
    if (options.style() == FloatParseOptions::Style::Scientific) {
        const auto parsedText = text.substr(0U, result.parsedLength);
        if (parsedText.find_first_of("eE") == std::string_view::npos) {
            return FloatParserResult{.value = {}, .parsedLength = 0U, .error = std::errc::invalid_argument};
        }
    }
    return result;
}
#endif

auto parseDoubleText(std::string_view text, const FloatParseOptions &options) -> FloatParserResult {
#if defined(_LIBCPP_VERSION)
    return parseDoubleWithStrtod(text, options);
#else
    auto value = double{};
    const auto *first = text.data();
    const auto *last = first + text.size();
    const auto [ptr, error] = std::from_chars(first, last, value, charsFormat(options.style()));
    return FloatParserResult{
        .value = value,
        .parsedLength = static_cast<std::size_t>(ptr - first),
        .error = error,
    };
#endif
}

auto parseDoubleCore(StringCharReader reader, const FloatParseOptions &options) -> FloatParseResult {
    const auto textResult = readFloatText(std::move(reader));
    if (textResult.status != FloatParseStatus::Success) {
        return floatParseFailure(textResult.status, textResult.message);
    }

    const auto &text = textResult.text;
    auto first = text.data();
    const auto last = first + text.size();
    if (first == last) {
        return floatParseFailure(FloatParseStatus::ParseError, "Floating point text is empty");
    }
    if (*first == '+') {
        ++first;
    }

    const auto parseResult = parseDoubleText(std::string_view{first, last}, options);
    const auto error = parseResult.error;
    if (error == std::errc::invalid_argument) {
        return floatParseFailure(FloatParseStatus::ParseError, "Floating point text contains no floating point value");
    }
    if (error == std::errc::result_out_of_range) {
        return floatParseFailure(FloatParseStatus::Overflow, "Floating point text exceeds the requested type");
    }
    if (!options.hasFlag(FloatParseFlag::IgnoreTrailingChars) &&
        parseResult.parsedLength != std::size_t(last - first)) {
        return floatParseFailure(FloatParseStatus::ParseError, "Floating point text has trailing characters");
    }
    return floatParseSuccess(parseResult.value);
}

auto parseDoubleOrDefault(StringCharReader reader, const double defaultValue, const FloatParseOptions &options) noexcept
    -> double {
    try {
        const auto result = parseDoubleCore(std::move(reader), options);
        if (result.status != FloatParseStatus::Success) {
            return defaultValue;
        }
        return result.value;
    } catch (...) {
        return defaultValue;
    }
}

auto parseDoubleOrThrow(StringCharReader reader, const FloatParseOptions &options) -> double {
    const auto result = parseDoubleCore(std::move(reader), options);
    switch (result.status) {
    case FloatParseStatus::Success:
        return result.value;
    case FloatParseStatus::ParseError:
        text::impl::throwParseError(result.message);
    case FloatParseStatus::Overflow:
        text::impl::throwOverflow(result.message);
    }
    text::impl::throwParseError("Floating point text could not be parsed");
}

[[nodiscard]] auto isFloatRepresentable(const double value) noexcept -> bool {
    if (!std::isfinite(value) || value == 0.0) {
        return true;
    }
    const auto magnitude = std::abs(value);
    return magnitude <= static_cast<double>(std::numeric_limits<float>::max()) &&
        magnitude >= static_cast<double>(std::numeric_limits<float>::denorm_min());
}

auto parseFloatOrDefault(StringCharReader reader, const float defaultValue, const FloatParseOptions &options) noexcept
    -> float {
    try {
        const auto value = parseDoubleOrThrow(std::move(reader), options);
        if (!isFloatRepresentable(value)) {
            return defaultValue;
        }
        return static_cast<float>(value);
    } catch (...) {
        return defaultValue;
    }
}

auto parseFloatOrThrow(StringCharReader reader, const FloatParseOptions &options) -> float {
    const auto value = parseDoubleOrThrow(std::move(reader), options);
    if (!isFloatRepresentable(value)) {
        text::impl::throwOverflow("Floating point text exceeds the requested type");
    }
    return static_cast<float>(value);
}

}
