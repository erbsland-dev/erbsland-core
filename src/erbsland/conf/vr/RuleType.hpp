// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../ValueType.hpp"

#include "../../text/String.hpp"
#include "../../util/impl/ComparisonHelper.hpp"

#include <array>
#include <cstdint>
#include <unordered_map>
#include <utility>

namespace erbsland::conf::vr {

class RuleType {
public:
    enum Enum : uint8_t {
        Undefined,        ///< Undefined validation type.
        Integer,          ///< An integer value.
        Boolean,          ///< A boolean value.
        Float,            ///< A floating-point value.
        Text,             ///< A text value.
        Date,             ///< A date value.
        Time,             ///< A time value.
        DateTime,         ///< A date-time value.
        Bytes,            ///< Binary data.
        TimeDelta,        ///< A time delta value.
        RegEx,            ///< A regular expression value.
        Value,            ///< A scalar value.
        ValueList,        ///< A list of values.
        ValueMatrix,      ///< A matrix of values.
        Section,          ///< A section with names.
        SectionList,      ///< A list of sections.
        SectionWithTexts, ///< A section with texts.
        NotValidated,     ///< A node that is not validated.
        Alternatives,     ///< A node that holds alternatives (OR conditions).
    };

private:
    struct Entry {
        Enum value;
        text::String text;
        ValueType valueType;
        text::String expectedValueTypeText;
    };
    using ValueMap = std::array<Entry, 19>;
    using TextToValueMap = std::unordered_map<text::String, Enum>;

public:
    /// @name Construction and Assignment
    /// @{

    /// Create an undefined value type.
    RuleType() = default;

    /// Create a new value type.
    /// @param value The enum value.
    RuleType(const Enum value) : _value(value) {} // NOLINT(*-explicit-constructor)

    /// @}

    // defaults
    RuleType(const RuleType &) = default;
    ~RuleType() = default;
    auto operator=(const RuleType &) -> RuleType & = default;

public: // operators
    /// @name Operators
    /// @{

    /// Assign an enum.
    auto operator=(const Enum value) noexcept -> RuleType & {
        _value = value;
        return *this;
    }
    /// Cast back to the enum value.
    constexpr operator Enum() const noexcept { return _value; } // NOLINT(*-explicit-constructor)

    /// @}

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const RuleType &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Enum value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(Enum a, const RuleType &b, a, b._value);

public: // tests
    /// @name Tests
    /// @{

    /// Test if the type is undefined.
    [[nodiscard]] constexpr auto isUndefined() const noexcept -> bool { return _value == Undefined; }
    /// Test if this type represents any kind of list.
    [[nodiscard]] auto isList() const noexcept -> bool {
        return _value == SectionList || _value == ValueList || _value == ValueMatrix;
    }
    /// Test if this type represents a scalar value.
    [[nodiscard]] auto isScalar() const noexcept -> bool {
        return _value == Integer || _value == Boolean || _value == Text || _value == Float || _value == Date ||
            _value == Time || _value == DateTime || _value == Bytes || _value == TimeDelta || _value == RegEx ||
            _value == Value;
    }
    /// Test if this rule type accepts defaults.
    [[nodiscard]] auto acceptsDefaults() const noexcept -> bool {
        return _value != Section && _value != SectionList && _value != SectionWithTexts && _value != NotValidated &&
            _value != Alternatives;
    }
    /// Test if this rule type matches a value type.
    [[nodiscard]] auto matchesValueType(ValueType valueType) const noexcept -> bool;

    /// @}

public: // conversion
    /// Convert this type into text.
    [[nodiscard]] auto toText() const noexcept -> const text::String &;
    /// Convert this type into a value type.
    [[nodiscard]] auto toValueType() const noexcept -> ValueType;
    /// Get the expected type text for this rule type.
    [[nodiscard]] auto expectedValueTypeText() const noexcept -> const text::String &;
    /// Create a validation type from a given text.
    /// @return The validation type or `Undefined` if the text does not match any valid type.
    [[nodiscard]] static auto fromText(const text::String &text) noexcept -> RuleType;

public:
    /// Access the underlying enum value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Enum { return _value; }

public: // enumeration
    /// Get an array with all value types.
    [[nodiscard]] static auto all() noexcept -> const std::array<RuleType, 19> &;

private:
    [[nodiscard]] static auto valueToTextMap() noexcept -> const ValueMap &;
    [[nodiscard]] static auto textToValueMap() noexcept -> const TextToValueMap &;
    [[nodiscard]] auto entry() const noexcept -> const Entry &;

private:
    Enum _value{Undefined};
};

}
