// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AnyString_fwd.hpp"
#include "AnyStringView_fwd.hpp"
#include "StringConverter.hpp"
#include "StringKind.hpp"

#include "u16/U16String.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringView.hpp"

#include "../unit/CpLength.hpp"

#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace erbsland::text {

template <typename tStringView>
concept AnyStringViewType = std::is_same_v<tStringView, U8StringView> || std::is_same_v<tStringView, U16StringView> ||
    std::is_same_v<tStringView, U32StringView>;

/// A wrapper that stores a string view of any type.
/// @tested{AnyStringTest}
class AnyStringView final {
    using Value = std::variant<std::monostate, U8StringView, U16StringView, U32StringView>;

public:
    /// Create an empty string view of an undefined type.
    AnyStringView() = default;
    /// Create a string view of the given type.
    template <AnyStringViewType tStringView>
    AnyStringView(tStringView str) noexcept : // NOLINT(*-explicit-constructor)
        _value(str.isEmpty() ? Value{} : std::move(str)) {}
    /// Create a string view from a UTF-8 string.
    AnyStringView(const U8String &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyStringView{U8StringView{str}} {}
    /// Create a string view from a UTF-16 string.
    AnyStringView(const U16String &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyStringView{U16StringView{str}} {}
    /// Create a string view from a UTF-32 string.
    AnyStringView(const U32String &str) noexcept : // NOLINT(*-explicit-constructor)
        AnyStringView{U32StringView{str}} {}
    /// Create an view from an AnyString
    AnyStringView(const AnyString &str) noexcept; // NOLINT(*-explicit-constructor)

    // defaults
    ~AnyStringView() = default;
    AnyStringView(const AnyStringView &) = default;
    AnyStringView(AnyStringView &&) = default;
    AnyStringView(U8String &&str) = delete;
    AnyStringView(U16String &&str) = delete;
    AnyStringView(U32String &&str) = delete;
    auto operator=(const AnyStringView &) -> AnyStringView & = default;
    auto operator=(AnyStringView &&) -> AnyStringView & = default;

public: // operators
    auto operator=(const U8StringView &str) -> AnyStringView & {
        _value = str;
        return *this;
    }
    auto operator=(U8StringView &&str) -> AnyStringView & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U16StringView &str) -> AnyStringView & {
        _value = str;
        return *this;
    }
    auto operator=(U16StringView &&str) -> AnyStringView & {
        _value = std::move(str);
        return *this;
    }
    auto operator=(const U32StringView &str) -> AnyStringView & {
        _value = str;
        return *this;
    }
    auto operator=(U32StringView &&str) -> AnyStringView & {
        _value = std::move(str);
        return *this;
    }

public: // accessors
    /// Test if this string view is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return std::holds_alternative<std::monostate>(_value); }
    /// Test if this string view does not require conversion to the given target kind.
    [[nodiscard]] auto noConversionForKind(const StringKind kind) const noexcept -> bool {
        if (std::holds_alternative<std::monostate>(_value)) {
            return true; // empty is of any kind.
        }
        if (this->kind().has_value() && this->kind().value() == kind) {
            return true;
        }
        return false;
    }
    /// Get the kind of the underlying string view.
    /// Returns no value for empty string views.
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
    /// Count the number of valid and replacement code points in the string view.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength {
        return std::visit(
            []<typename T>(const T &value) noexcept -> unit::CpLength {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8StringView> || std::is_same_v<ValueType, U16StringView>) {
                    return value.characterLength();
                } else if constexpr (std::is_same_v<ValueType, U32StringView>) {
                    return value.length();
                } else {
                    return unit::CpLength::zero();
                }
            },
            _value);
    }
    /// Test if the underlying string view contains only valid code points for its encoding.
    [[nodiscard]] auto isEncodingValid() const noexcept -> bool {
        return std::visit(
            []<typename T>(const T &value) noexcept -> bool {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8StringView>) {
                    return value.isValidUtf8();
                } else if constexpr (std::is_same_v<ValueType, U16StringView>) {
                    return value.isValidUtf16();
                } else if constexpr (std::is_same_v<ValueType, U32StringView>) {
                    return value.isValidUtf32();
                } else {
                    return true;
                }
            },
            _value);
    }

public: // conversion
    /// Get or convert this string view in U8 format.
    [[nodiscard]] auto toU8String() const -> U8String {
        return std::visit(
            []<typename T>(const T &value) -> U8String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8StringView>) {
                    return value.copy();
                } else if constexpr (AnyStringViewType<ValueType>) {
                    return StringConverter{value}.toU8String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string view in U16 format.
    [[nodiscard]] auto toU16String() const -> U16String {
        return std::visit(
            []<typename T>(const T &value) -> U16String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U16StringView>) {
                    return value.copy();
                } else if constexpr (AnyStringViewType<ValueType>) {
                    return StringConverter{value}.toU16String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string view in U32 format.
    [[nodiscard]] auto toU32String() const -> U32String {
        return std::visit(
            []<typename T>(const T &value) -> U32String {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U32StringView>) {
                    return value.copy();
                } else if constexpr (AnyStringViewType<ValueType>) {
                    return StringConverter{value}.toU32String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string view in U8 format.
    [[nodiscard]] auto toU8StringView() const -> U8StringView {
        return std::visit(
            []<typename T>(const T &value) -> U8StringView {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U8StringView>) {
                    return value;
                } else if constexpr (AnyStringViewType<ValueType>) {
                    return StringConverter{value}.toU8String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string view in U16 format.
    [[nodiscard]] auto toU16StringView() const -> U16StringView {
        return std::visit(
            []<typename T>(const T &value) -> U16StringView {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U16StringView>) {
                    return value;
                } else if constexpr (AnyStringViewType<ValueType>) {
                    return StringConverter{value}.toU16String();
                } else {
                    return {};
                }
            },
            _value);
    }
    /// Get or convert this string view in U32 format.
    [[nodiscard]] auto toU32StringView() const -> U32StringView {
        return std::visit(
            []<typename T>(const T &value) -> U32StringView {
                using ValueType = std::remove_cvref_t<T>;
                if constexpr (std::is_same_v<ValueType, U32StringView>) {
                    return value;
                } else if constexpr (AnyStringViewType<ValueType>) {
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
