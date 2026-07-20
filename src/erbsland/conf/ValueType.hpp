// Copyright (c) 2024-2025 Erbsland DEV. https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../mem/ByteBlock_fwd.hpp"
#include "../re/RegEx_fwd.hpp"
#include "../text/String.hpp"
#include "../text/StringEditor.hpp"
#include "../time/CalendarDelta.hpp"
#include "../time/Date.hpp"
#include "../time/DateTime.hpp"
#include "../time/Time.hpp"
#include "../time/TimeWithZone.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <array>
#include <cstdint>

namespace erbsland::conf {

/// The type of value.
class ValueType final {
public:
    /// The enum for this type.
    enum Enum : uint8_t {
        Undefined,           ///< Undefined type.
        Integer,             ///< An integer value.
        Boolean,             ///< A boolean value.
        Float,               ///< A floating-point value.
        Text,                ///< A text value.
        Date,                ///< A date value.
        Time,                ///< A time value.
        DateTime,            ///< A date-time value.
        Bytes,               ///< Binary data.
        TimeDelta,           ///< A time delta value.
        RegEx,               ///< A regular expression value.
        ValueList,           ///< A list of values.
        SectionList,         ///< A list of sections.
        IntermediateSection, ///< An intermediate section.
        SectionWithNames,    ///< A section with names.
        SectionWithTexts,    ///< A section with texts.
        Document,            ///< The document.
    };

public:
    /// @name Construction and Assignment
    /// @{

    /// Create an undefined value type.
    constexpr ValueType() = default;

    /// Create a new value type.
    /// @param value The enum value.
    constexpr ValueType(const Enum value) noexcept : _value{value} {} // NOLINT(*-explicit-constructor)

    /// @}

    // defaults
    ~ValueType() = default;
    ValueType(const ValueType &) = default;
    auto operator=(const ValueType &) -> ValueType & = default;

public: // operators
    /// @name Operators
    /// @{

    /// Assign an enum.
    auto operator=(const Enum value) noexcept -> ValueType & {
        _value = value;
        return *this;
    }
    /// Cast back to the enum value.
    constexpr operator Enum() const noexcept { return _value; } // NOLINT(*-explicit-constructor)

    /// @}

public: // comparison
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, const ValueType &other, other._value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_MEMBER(_value, Enum value, value);
    ERBSLAND_CORE_CONSTEXPR_COMPARE_FRIEND(Enum a, const ValueType &b, a, b._value);

public: // tests
    /// @name Tests
    /// @{

    /// Test if the type is undefined.
    [[nodiscard]] constexpr auto isUndefined() const noexcept -> bool { return _value == Undefined; }
    /// Test if this is any kind of value map (a section or document).
    [[nodiscard]] constexpr auto isMap() const noexcept -> bool {
        return _value == SectionWithTexts || _value == SectionWithNames || _value == IntermediateSection ||
            _value == Document;
    }
    /// Test if this is any kind of list (section list or value list).
    [[nodiscard]] constexpr auto isList() const noexcept -> bool {
        return _value == ValueList || _value == SectionList;
    }
    /// Test if this is a structural value.
    /// Structural values are documents, sections and section lists that organize the content of the document.
    [[nodiscard]] constexpr auto isStructural() const noexcept -> bool {
        return _value == SectionWithTexts || _value == SectionWithNames || _value == IntermediateSection ||
            _value == SectionList || _value == Document;
    }
    /// Test if this is a scalar value.
    /// A scalar value represents a single value (not a section, section list or value list).
    /// Scalar values are: Integer, Boolean, Float, Text, Date, Time, DateTime, mem::ByteBlock, TimeDelta and RegEx.
    [[nodiscard]] constexpr auto isScalar() const noexcept -> bool {
        return _value == Integer || _value == Boolean || _value == Float || _value == Text || _value == Date ||
            _value == Time || _value == DateTime || _value == Bytes || _value == TimeDelta || _value == RegEx;
    }
    /// @deprecated Please use `isScalar()` for new code.
    [[nodiscard]] constexpr auto isSingle() const noexcept -> bool { return isScalar(); }

    /// @}

public: // transformation
    /// Convert this type into text.
    [[nodiscard]] auto toText() const noexcept -> text::String;
    /// Convert this type into a value description for error messages.
    /// This method creates a human-readable description of this value type, describing the value for
    /// error reporting and user facing texts.
    /// @param withArticle Add an English article to the text.
    [[nodiscard]] auto toValueDescription(bool withArticle) const -> text::String;
    /// Get a value type for a single value, native type.
    /// Please note that this method does not support sections or value lists.
    template <typename T>
    [[nodiscard]] constexpr static auto from() noexcept -> ValueType {
        using BaseT = std::remove_cvref_t<T>;
        if constexpr (std::is_same_v<BaseT, bool>) {
            return Boolean;
        } else if constexpr (std::is_integral_v<BaseT>) {
            return Integer;
        } else if constexpr (std::is_floating_point_v<BaseT>) {
            return Float;
        } else if constexpr (std::is_same_v<BaseT, text::String> || std::is_same_v<BaseT, text::StringEditor>) {
            return Text;
        } else if constexpr (std::is_same_v<BaseT, time::Date>) {
            return Date;
        } else if constexpr (std::is_same_v<BaseT, time::Time> || std::is_same_v<BaseT, time::TimeWithZone>) {
            return Time;
        } else if constexpr (std::is_same_v<BaseT, time::DateTime>) {
            return DateTime;
        } else if constexpr (std::is_same_v<BaseT, time::CalendarDelta>) {
            return TimeDelta;
        } else if constexpr (std::is_same_v<BaseT, re::RegExPtr>) {
            return RegEx;
        } else if constexpr (std::is_same_v<BaseT, mem::ByteBlock>) {
            return Bytes;
        } else {
            return Undefined;
        }
    }

public:
    /// Access the underlying enum value.
    [[nodiscard]] constexpr auto raw() const noexcept -> Enum { return _value; }

public: // enumeration
    /// Get an array with all value types.
    [[nodiscard]] static auto all() noexcept -> const std::array<ValueType, 17> &;

private:
    Enum _value{Undefined}; ///< The internal value.
    using ValueToTextEntry = std::tuple<Enum, text::String>;
    using ValueToTextMap = std::array<ValueToTextEntry, 17>;
    static ValueToTextMap _valueToTextMap; ///< A for value and text translations.
};

}

template <>
struct std::hash<erbsland::conf::ValueType> {
    auto operator()(const erbsland::conf::ValueType &unit) const noexcept -> std::size_t {
        return std::hash<std::underlying_type_t<erbsland::conf::ValueType::Enum>>{}(
            static_cast<erbsland::conf::ValueType::Enum>(unit));
    }
};
