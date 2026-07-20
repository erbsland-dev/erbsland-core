// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString.hpp"
#include "AnyStringEditor_fwd.hpp"
#include "StringConverter.hpp"
#include "StringKind.hpp"

#include "u16/U16StringEditor.hpp"
#include "u32/U32StringEditor.hpp"
#include "u8/U8StringEditor.hpp"

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::text {

template <typename tString>
concept AnyStringEditorType = std::is_same_v<tString, U8StringEditor> || std::is_same_v<tString, U16StringEditor> ||
    std::is_same_v<tString, U32StringEditor>;

/// A wrapper that stores a mutable string editor of any supported width.
/// Converts its contents to a requested read-only string width when needed.
/// @tested{AnyStringTest}
class AnyStringEditor final {
    using Value = std::variant<std::monostate, U8StringEditor, U16StringEditor, U32StringEditor>;

public:
    /// Create an empty string of an undefined type.
    AnyStringEditor() = default;

    /// Create a string of the given type.
    template <AnyStringEditorType tString>
    constexpr AnyStringEditor(tString str) : // NOLINT(*-explicit-constructor)
        _value(str.isEmpty() ? Value{} : std::move(str)) {}

    // defaults
    ~AnyStringEditor() = default;
    AnyStringEditor(const AnyStringEditor &) = default;
    AnyStringEditor(AnyStringEditor &&) = default;
    auto operator=(const AnyStringEditor &) -> AnyStringEditor & = default;
    auto operator=(AnyStringEditor &&) -> AnyStringEditor & = default;

public: // operators
    auto operator=(const U8StringEditor &str) -> AnyStringEditor & {
        _value = str;
        return *this;
    }
    auto operator=(U8StringEditor &&str) -> AnyStringEditor & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U16StringEditor &str) -> AnyStringEditor & {
        _value = str;
        return *this;
    }
    auto operator=(U16StringEditor &&str) -> AnyStringEditor & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U32StringEditor &str) -> AnyStringEditor & {
        _value = str;
        return *this;
    }
    auto operator=(U32StringEditor &&str) -> AnyStringEditor & {
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
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength { return toAnyString().characterLength(); }
    /// Test if the underlying string contains only valid code points for its encoding.
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool { return toAnyString().isEncodingValid(); }

public: // conversion
    /// Create an owning read-only value sharing the stored editor contents.
    [[nodiscard]] auto toAnyString() const noexcept -> AnyString {
        return std::visit(
            []<typename T>(const T &value) noexcept -> AnyString {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (
                    std::is_same_v<ValueType, U8StringEditor> || std::is_same_v<ValueType, U16StringEditor> ||
                    std::is_same_v<ValueType, U32StringEditor>) {
                    return AnyString{value};
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
                if constexpr (std::is_same_v<ValueType, U8StringEditor>) {
                    return value;
                } else if constexpr (std::is_same_v<ValueType, U16StringEditor>) {
                    return StringConverter{value}.toU8String();
                } else if constexpr (std::is_same_v<ValueType, U32StringEditor>) {
                    return StringConverter{value}.toU8String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// @overload
    [[nodiscard]] auto toString() const -> String { return toU8String(); }
    /// Get or convert this string in U16 format.
    [[nodiscard]] auto toU16String() const -> U16String {
        return std::visit(
            []<typename T>(T &&value) -> U16String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8StringEditor>) {
                    return StringConverter{value}.toU16String();
                } else if constexpr (std::is_same_v<ValueType, U16StringEditor>) {
                    return value;
                } else if constexpr (std::is_same_v<ValueType, U32StringEditor>) {
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
                if constexpr (std::is_same_v<ValueType, U8StringEditor>) {
                    return StringConverter{value}.toU32String();
                } else if constexpr (std::is_same_v<ValueType, U16StringEditor>) {
                    return StringConverter{value}.toU32String();
                } else if constexpr (std::is_same_v<ValueType, U32StringEditor>) {
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
