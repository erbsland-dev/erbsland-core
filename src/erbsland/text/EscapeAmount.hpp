// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char_fwd.hpp"
#include "String_fwd.hpp"

#include "../util/impl/ComparisonHelper.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::text {

/// What range of characters shall get escaped.
/// @tested{StringEscapingTest}
class EscapeAmount final {
public:
    /// The escape amount value.
    enum Value : uint8_t {
        Nothing = 0,    ///< Escape nothing.
        Required = 1,   ///< Escape only required characters.
        Balanced = 2,   ///< Escape invisible and control characters.
        NonAscii = 3,   ///< Escape everything, except visible ASCII characters.
        Everything = 4, ///< Escape everything.
    };

public:
    /// Create an escape amount from a value.
    constexpr EscapeAmount(const Value value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    // defaults
    constexpr EscapeAmount() noexcept = default;
    ~EscapeAmount() = default;
    EscapeAmount(const EscapeAmount &) = default;
    EscapeAmount(EscapeAmount &&) = default;
    auto operator=(const EscapeAmount &) -> EscapeAmount & = default;
    auto operator=(EscapeAmount &&) -> EscapeAmount & = default;

public: // operators
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const EscapeAmount &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const Value value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(const Value value, const EscapeAmount &other, value, other._value);

public: // accessors
    /// Get the raw escape amount value.
    [[nodiscard]] constexpr auto toRawValue() const noexcept -> Value { return _value; }

public: // conversion
    /// Convert this escape amount to its canonical string.
    [[nodiscard]] auto toString() const -> StringView;
    /// Create an escape amount from a canonical string.
    [[nodiscard]] static auto fromString(const StringView &text) noexcept -> std::optional<EscapeAmount>;
    /// Create an escape amount from a canonical string.
    /// @throws err::ParseError if the string is not a supported escape amount.
    [[nodiscard]] static auto fromStringOrThrow(const StringView &text) -> EscapeAmount;
    /// Create an escape amount from a format suffix character.
    [[nodiscard]] static auto fromSuffix(Char character) noexcept -> std::optional<EscapeAmount>;

private:
    Value _value{Nothing}; ///< The escape amount value.
};

}
