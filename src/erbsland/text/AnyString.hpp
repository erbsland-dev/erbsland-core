// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringView.hpp"
#include "StringConverter.hpp"
#include "StringKind.hpp"

#include "u16/U16String.hpp"
#include "u32/U32String.hpp"
#include "u8/U8String.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::text {

template <typename tString>
concept AnyStringType =
    std::is_same_v<tString, U8String> || std::is_same_v<tString, U16String> || std::is_same_v<tString, U32String>;

/// A wrapper that stores a string of any type.
/// Automatically converts to the underlying string type into the requested target format.
/// @tested{AnyStringTest}
class AnyString final {
    using Value = std::variant<std::monostate, U8String, U16String, U32String>;

public:
    /// Create an empty string of an undefined type.
    AnyString() = default;

    /// Create a string of the given type.
    template <AnyStringType tString>
    constexpr AnyString(tString str) : // NOLINT(*-explicit-constructor)
        _value(str.isEmpty() ? Value{} : std::move(str)) {}

    // defaults
    ~AnyString() = default;
    AnyString(const AnyString &) = default;
    AnyString(AnyString &&) = default;
    auto operator=(const AnyString &) -> AnyString & = default;
    auto operator=(AnyString &&) -> AnyString & = default;

public: // operators
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

public:
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return std::holds_alternative<std::monostate>(_value); }

    /// Test if this string does not require conversion to the given target kind.
    [[nodiscard]] auto noConversionForKind(const StringKind kind) const noexcept -> bool {
        if (std::holds_alternative<std::monostate>(_value)) {
            return true; // empty is of any kind.
        }
        if (this->kind().has_value() && this->kind().value() == kind) {
            return true;
        }
        return false;
    }

    /// Get the kind of the underlying string.
    /// Returns no value for empty strings.
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
    /// Count the number of valid and replacement code points in the string.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength {
        return toAnyStringView().characterLength();
    }
    /// Test if the underlying string contains only valid code points for its encoding.
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool { return toAnyStringView().isEncodingValid(); }

public: // conversion
    /// Create a view to the stored string.
    [[nodiscard]] auto toAnyStringView() const noexcept -> AnyStringView {
        return std::visit(
            []<typename T>(const T &value) noexcept -> AnyStringView {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (
                    std::is_same_v<ValueType, U8String> || std::is_same_v<ValueType, U16String> ||
                    std::is_same_v<ValueType, U32String>) {
                    return AnyStringView{value};
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string in U8 format.
    [[nodiscard]] auto toU8String() const -> U8String {
        return std::visit(
            []<typename T>(T &&value) -> U8String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String>) {
                    return value;
                } else if constexpr (std::is_same_v<ValueType, U16String>) {
                    return StringConverter{value}.toU8String();
                } else if constexpr (std::is_same_v<ValueType, U32String>) {
                    return StringConverter{value}.toU8String();
                } else {
                    return {};
                }
            },
            _value);
    }

    /// Get or convert this string in U16 format.
    [[nodiscard]] auto toU16String() const -> U16String {
        return std::visit(
            []<typename T>(T &&value) -> U16String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String>) {
                    return StringConverter{value}.toU16String();
                } else if constexpr (std::is_same_v<ValueType, U16String>) {
                    return value;
                } else if constexpr (std::is_same_v<ValueType, U32String>) {
                    return StringConverter{value}.toU16String();
                } else {
                    return {};
                }
            },
            _value);
    }

    /// Get or convert this string in U32 format.
    [[nodiscard]] auto toU32String() const -> U32String {
        return std::visit(
            []<typename T>(T &&value) -> U32String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8String>) {
                    return StringConverter{value}.toU32String();
                } else if constexpr (std::is_same_v<ValueType, U16String>) {
                    return StringConverter{value}.toU32String();
                } else if constexpr (std::is_same_v<ValueType, U32String>) {
                    return value;
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
