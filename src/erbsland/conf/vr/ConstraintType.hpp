// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <array>
#include <cstdint>

namespace erbsland::conf::vr {

/// Value object representing a constraint type.
class ConstraintType {
public:
    enum Enum : uint8_t {
        Undefined,      ///< Undefined constraint type, to flag errors.
        Chars,          ///< Constraining characters.
        Contains,       ///< Test if text contained in another.
        Ends,           ///< Test if text ends with a sequence.
        Equals,         ///< Test if a value equals a constant or size.
        In,             ///< Test if a value is in a list.
        ConfKey,        ///< Test if key is in index.
        Matches,        ///< Test a text using a regular expression.
        Maximum,        ///< Limit maximum.
        MaximumVersion, ///< Limit maximum version.
        Minimum,        ///< Limit minimum.
        MinimumVersion, ///< Limit minimum version.
        Multiple,       ///< Test if a value is a multiple of.
        Starts,         ///< Test if a value starts with a sequence.
        ConfVersion,    ///< Test if a version matches.
    };

public:
    /// @name Construction and Assignment
    /// @{

    /// Create an undefined value type.
    ConstraintType() = default;

    /// Create a new value type.
    /// @param value The enum value.
    ConstraintType(const Enum value) : _value(value) {} // NOLINT(*-explicit-constructor)

    /// @}

    // defaults
    ConstraintType(const ConstraintType &) = default;
    ~ConstraintType() = default;
    auto operator=(const ConstraintType &) -> ConstraintType & = default;

public: // operators
    /// @name Operators
    /// @{

    /// Assign an enum.
    auto operator=(const Enum value) noexcept -> ConstraintType & {
        _value = value;
        return *this;
    }
    /// Cast back to the enum value.
    constexpr operator Enum() const noexcept { return _value; } // NOLINT(*-explicit-constructor)

    /// @}

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ConstraintType &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Enum value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(Enum a, const ConstraintType &b, a, b._value);

public: // tests
    /// @name Tests
    /// @{

    /// Test if the type is undefined.
    [[nodiscard]] constexpr auto isUndefined() const noexcept -> bool { return _value == Undefined; }

    /// @}

public: // conversion
    /// Convert this type into text.
    [[nodiscard]] auto toText() const noexcept -> const text::String &;

    /// Create a validation type from a given text.
    /// @return The validation type or `Undefined` if the text does not match any valid type.
    [[nodiscard]] static auto fromText(const text::String &text) noexcept -> ConstraintType;

public:
    /// Access the underlying enum value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Enum { return _value; }

public: // enumeration
    /// Get an array with all value types.
    [[nodiscard]] static auto all() noexcept -> const std::array<ConstraintType, 15> & {
        static const std::array<ConstraintType, 15> values = {
            Undefined,
            Chars,
            Contains,
            Ends,
            Equals,
            In,
            ConfKey,
            Matches,
            Maximum,
            MaximumVersion,
            Minimum,
            MinimumVersion,
            Multiple,
            Starts,
            ConfVersion};
        return values;
    }

private:
    Enum _value{Undefined};
    /// Mapping entry from a constraint type value to text.
    struct ValueToTextEntry {
        Enum value;
        text::String text;
    };
    using ValueToTextMap = std::array<ValueToTextEntry, 15>;
    using TextToValueMap = std::unordered_map<text::String, Enum>;
    static ValueToTextMap _valueToTextMap;
    static TextToValueMap _textToValueMap;
};

}
