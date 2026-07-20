// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringEditor_fwd.hpp"
#include "CharCompareFn.hpp"
#include "StringConverter.hpp"
#include "StringKind.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringEditor.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringEditor.hpp"

#include "../unit/CpLength.hpp"
#include "../util/impl/ComparisonHelper.hpp"

#include <compare>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::text {

template <typename tString>
concept AnyStringType =
    std::is_same_v<tString, U8String> || std::is_same_v<tString, U16String> || std::is_same_v<tString, U32String>;

/// A wrapper that stores a read-only string of any supported width.
/// @tested{AnyStringTest}
class AnyString final {
    using Value = std::variant<std::monostate, U8String, U16String, U32String>;

public:
    /// Create an empty string of an undefined type.
    AnyString() = default;
    /// Create a string of the given type.
    template <AnyStringType tString>
    AnyString(tString str) noexcept : // NOLINT(*-explicit-constructor)
        _value(str.isEmpty() ? Value{} : std::move(str)) {}
    /// Create a read-only string sharing a narrow UTF-8 literal.
    AnyString(const U8StringLiteral<char> &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U8String{str}} {}
    /// Create a read-only string sharing a UTF-8 literal.
    AnyString(const U8StringLiteral<char8_t> &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U8String{str}} {}
    /// Create a read-only string sharing a UTF-16 literal.
    AnyString(const U16StringLiteral &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U16String{str}} {}
    /// Create a read-only string sharing a UTF-32 literal.
    AnyString(const U32StringLiteral &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U32String{str}} {}
    /// Create a read-only string from a UTF-8 editor.
    AnyString(const U8StringEditor &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U8String{str}} {}
    /// Create a read-only string from a UTF-16 editor.
    AnyString(const U16StringEditor &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U16String{str}} {}
    /// Create a read-only string from a UTF-32 editor.
    AnyString(const U32StringEditor &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyString{U32String{str}} {}
    /// Create a read-only string from an `AnyStringEditor`.
    AnyString(const AnyStringEditor &str) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    ~AnyString() = default;
    AnyString(const AnyString &) = default;
    AnyString(AnyString &&) = default;
    auto operator=(const AnyString &) -> AnyString & = default;
    auto operator=(AnyString &&) -> AnyString & = default;

public: // operators
    /// Compare two strings by decoded code point.
    [[nodiscard]] auto operator<=>(const AnyString &other) const noexcept -> std::strong_ordering;
    ERBSLAND_CORE_COMPARE_FROM_SPACESHIP(const AnyString &other, other);
    auto operator=(const U8String &str) -> AnyString & {
        _value = str;
        return *this;
    }
    auto operator=(U8String &&str) -> AnyString & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U16String &str) -> AnyString & {
        _value = str;
        return *this;
    }
    auto operator=(U16String &&str) -> AnyString & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U32String &str) -> AnyString & {
        _value = str;
        return *this;
    }
    auto operator=(U32String &&str) -> AnyString & {
        _value = std::move(str);
        return *this;
    }

public: // comparison
    /// Compare two strings by decoded code point, replacing malformed encoding with `Char::replacement()`.
    /// The original string widths are preserved and no converted strings are created.
    /// @param other The string to compare with.
    /// @param compareFn The optional character comparison function.
    /// @return A three-way comparison result.
    [[nodiscard]] auto compare(const AnyString &other, CharCompareFn compareFn = {}) const noexcept
        -> std::strong_ordering;

public: // accessors
    /// Test if this read-only string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return std::holds_alternative<std::monostate>(_value); }
    /// Test if this read-only string does not require conversion to the given target kind.
    [[nodiscard]] auto noConversionForKind(const StringKind kind) const noexcept -> bool {
        if (std::holds_alternative<std::monostate>(_value)) {
            return true; // empty is of any kind.
        }
        if (this->kind().has_value() && this->kind().value() == kind) {
            return true;
        }
        return false;
    }
    /// Get the kind of the underlying read-only string.
    /// Returns no value for empty read-only strings.
    [[nodiscard]] auto kind() const noexcept -> std::optional<StringKind> {
        switch (_value.index()) {
        default:
            return {};
        case 1:
            return StringKind::U8;
        case 2:
            return StringKind::U16;
        case 3:
            return StringKind::U32;
        }
    }
    /// Count the number of valid and replacement code points in the read-only string.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength {
        return std::visit(
            []<typename T>(const T &value) noexcept -> unit::CpLength {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String> || std::is_same_v<ValueType, U16String>) {
                    return value.characterLength();
                } else if constexpr (std::is_same_v<ValueType, U32String>) {
                    return value.length();
                } else {
                    return unit::CpLength::zero();
                }
            },
            _value);
    }
    /// Test if the underlying read-only string contains only valid code points for its encoding.
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool {
        return std::visit(
            []<typename T>(const T &value) noexcept -> bool {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String>) {
                    return value.isValidUtf8();
                } else if constexpr (std::is_same_v<ValueType, U16String>) {
                    return value.isValidUtf16();
                } else if constexpr (std::is_same_v<ValueType, U32String>) {
                    return value.isValidUtf32();
                } else {
                    return true;
                }
            },
            _value);
    }

public: // conversion
    /// Get or convert this read-only string in U8 format.
    [[nodiscard]] auto toU8String() const -> U8String {
        return std::visit(
            []<typename T>(const T &value) -> U8String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String>) {
                    return value;
                } else if constexpr (AnyStringType<ValueType>) {
                    return StringConverter{value}.toU8String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// @overload
    [[nodiscard]] auto toString() const -> String { return toU8String(); }
    /// Get or convert this read-only string in U16 format.
    [[nodiscard]] auto toU16String() const -> U16String {
        return std::visit(
            []<typename T>(const T &value) -> U16String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U16String>) {
                    return value;
                } else if constexpr (AnyStringType<ValueType>) {
                    return StringConverter{value}.toU16String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this read-only string in U32 format.
    [[nodiscard]] auto toU32String() const -> U32String {
        return std::visit(
            []<typename T>(const T &value) -> U32String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U32String>) {
                    return value;
                } else if constexpr (AnyStringType<ValueType>) {
                    return StringConverter{value}.toU32String();
                } else {
                    return {};
                }
            },
            _value);
    }

private:
    Value _value;
};

}
