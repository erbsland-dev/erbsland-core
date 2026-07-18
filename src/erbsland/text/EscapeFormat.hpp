// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringEditor_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::text {

/// The target syntax for escaped text.
/// @tested{StringEscapingTest}
class EscapeFormat final {
public:
    /// The escape format value.
    enum Value : uint8_t {
        None = 0,    ///< Do not escape any characters.
        Html = 1,    ///< Escape for HTML text.
        Json = 2,    ///< Escape for JSON text.
        Cpp = 3,     ///< Escape for C++ literals.
        Xml = 4,     ///< Escape for XML text.
        RegEx = 5,   ///< Escape for regular expression literal patterns.
        Display = 6, ///< Escape unsafe characters for human-readable display text.
    };

public:
    /// Create an escape format from a value.
    constexpr EscapeFormat(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr EscapeFormat() noexcept = default;
    ~EscapeFormat() = default;
    EscapeFormat(const EscapeFormat &) = default;
    EscapeFormat(EscapeFormat &&) = default;
    auto operator=(const EscapeFormat &) -> EscapeFormat & = default;
    auto operator=(EscapeFormat &&) -> EscapeFormat & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const EscapeFormat &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const EscapeFormat &other, value, other._value);

public: // accessors
    /// Get the raw escape format value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this escape format to its canonical string.
    [[nodiscard]] auto toString() const -> String;
    /// Create an escape format from a canonical string.
    [[nodiscard]] static auto fromString(const String &text) noexcept -> std::optional<EscapeFormat>;
    /// Create an escape format from a canonical string.
    /// @throws err::ParseError if the string is not a supported escape format.
    [[nodiscard]] static auto fromStringOrThrow(const String &text) -> EscapeFormat;

private:
    Value _value{None}; ///< The escape format value.
};

}
