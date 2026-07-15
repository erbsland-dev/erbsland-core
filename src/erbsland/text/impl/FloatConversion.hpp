// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "FloatTraits.hpp"

#include "../FloatFormat.hpp"
#include "../FloatParseOptions.hpp"
#include "../String_fwd.hpp"
#include "../StringCharReader.hpp"

#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace erbsland::text::impl {

/// Format a floating point value into a temporary UTF-8 compatible standard string.
[[nodiscard]] auto formatFloat(double value, const FloatFormat &format) -> String;

/// The result status for floating point parsing.
enum class FloatParseStatus : uint8_t {
    Success,
    ParseError,
    Overflow,
};

/// Result for decoded text preparation before floating point parsing.
struct FloatTextResult final {
    std::string text;                                   ///< The validated UTF-8 text.
    FloatParseStatus status{FloatParseStatus::Success}; ///< The result status.
    const std::string_view message;                     ///< The error message for throwing wrappers.
};

/// Result for the shared floating point parser.
struct FloatParseResult final {
    double value{};                                     ///< The parsed value, only valid for success.
    FloatParseStatus status{FloatParseStatus::Success}; ///< The parser status.
    const std::string_view message;                     ///< The error message for throwing wrappers.
};

/// Result from the selected standard-library floating point parser.
struct FloatParserResult final {
    double value{};             ///< The parsed value.
    std::size_t parsedLength{}; ///< The number of accepted bytes.
    std::errc error{};          ///< The backend parser error.
};

/// Select the standard format character for the given float format.
[[nodiscard]] auto presentationType(const FloatFormat &format) noexcept -> char;

/// Build the `std::format` pattern for the given float format.
[[nodiscard]] auto formatPattern(const FloatFormat &format) -> std::string;

/// Validate decoded input and create UTF-8 text for the standard parser backend.
[[nodiscard]] auto readFloatText(StringCharReader reader) -> FloatTextResult;

/// Map Erbsland parse styles to the standard floating point parser format.
[[nodiscard]] auto charsFormat(FloatParseOptions::Style style) noexcept -> std::chars_format;

/// Create a successful parse result.
[[nodiscard]] auto floatParseSuccess(double value) noexcept -> FloatParseResult;

/// Create a failed parse result.
[[nodiscard]] auto floatParseFailure(FloatParseStatus status, std::string_view message) noexcept -> FloatParseResult;

#if defined(_LIBCPP_VERSION)
/// Test if the hexadecimal text already has a prefix accepted by `strtod`.
[[nodiscard]] auto hasHexFloatPrefix(std::string_view text) noexcept -> bool;

/// Insert the `strtod` hexadecimal prefix after the optional sign.
[[nodiscard]] auto addHexFloatPrefix(std::string text) -> std::string;

/// Convert the parsed offset from a prefixed hexadecimal fallback string back to the caller text.
[[nodiscard]] auto removeAddedHexPrefixFromLength(std::size_t parsedLength, bool hadMinus) noexcept -> std::size_t;

/// Parse floating point text through the C library when libc++ lacks floating `from_chars`.
[[nodiscard]] auto parseDoubleWithStrtodText(std::string_view text) -> FloatParserResult;

/// Parse floating point text through the C library when libc++ lacks floating `from_chars`.
[[nodiscard]] auto parseDoubleWithStrtod(std::string_view text, const FloatParseOptions &options) -> FloatParserResult;
#endif

/// Parse floating point text through the best available standard-library backend.
[[nodiscard]] auto parseDoubleText(std::string_view text, const FloatParseOptions &options) -> FloatParserResult;

/// Parse a floating point value from a decoded string reader as double.
[[nodiscard]] auto parseDoubleCore(StringCharReader reader, const FloatParseOptions &options) -> FloatParseResult;

/// Parse a floating point value from a decoded string reader as double, returning a default on failure.
[[nodiscard]] auto parseDoubleOrDefault(
    StringCharReader reader, double defaultValue, const FloatParseOptions &options) noexcept -> double;

/// Parse a floating point value from a decoded string reader as double.
[[nodiscard]] auto parseDoubleOrThrow(StringCharReader reader, const FloatParseOptions &options) -> double;

/// Test if a parsed double can be represented by the public float API.
[[nodiscard]] auto isFloatRepresentable(double value) noexcept -> bool;

/// Parse a floating point value from a decoded string reader as float, returning a default on failure.
[[nodiscard]] auto parseFloatOrDefault(
    StringCharReader reader, float defaultValue, const FloatParseOptions &options) noexcept -> float;

/// Parse a floating point value from a decoded string reader as float.
[[nodiscard]] auto parseFloatOrThrow(StringCharReader reader, const FloatParseOptions &options) -> float;

}
