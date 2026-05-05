// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Char.hpp"
#include "FormatArgumentKind.hpp"

#include "u16/U16StringView.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8StringView.hpp"

#include <cstdint>
#include <utility>
#include <variant>

namespace erbsland::text {

/// A small owning adapter for one runtime format argument.
/// @tested{U8FormatTest}
class FormatArgument final {
    // The alternative order must match the values of `FormatArgumentKind`.
    using Value =
        std::variant<std::monostate, U8StringView, U16StringView, U32StringView, int64_t, uint64_t, double, bool, Char>;

public:
    /// Create an empty format argument.
    FormatArgument() = default;
    /// Create a format argument by constructing the matching variant alternative.
    template <typename T>
        requires std::is_constructible_v<Value, T>
    explicit FormatArgument(T &&value) : _value{std::forward<T>(value)} {}

    // defaults
    ~FormatArgument() = default;
    FormatArgument(const FormatArgument &) = default;
    FormatArgument(FormatArgument &&) = default;
    auto operator=(const FormatArgument &) -> FormatArgument & = default;
    auto operator=(FormatArgument &&) -> FormatArgument & = default;

public: // accessors
    /// Get the argument kind.
    [[nodiscard]] auto kind() const noexcept -> FormatArgumentKind;
    /// Get the UTF-8 text argument.
    [[nodiscard]] auto u8Text() const -> U8StringView;
    /// Get the UTF-16 text argument.
    [[nodiscard]] auto u16Text() const -> U16StringView;
    /// Get the UTF-32 text argument.
    [[nodiscard]] auto u32Text() const -> U32StringView;
    /// Get the signed integer argument.
    [[nodiscard]] auto signedInteger() const -> int64_t;
    /// Get the unsigned integer argument.
    [[nodiscard]] auto unsignedInteger() const -> uint64_t;
    /// Get the floating point argument.
    [[nodiscard]] auto floatingPoint() const -> double;
    /// Get the boolean argument.
    [[nodiscard]] auto boolean() const -> bool;
    /// Get the character argument.
    [[nodiscard]] auto character() const -> Char;

private:
    Value _value; ///< The stored argument value.
};

}
