// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../core/Definitions.hpp"

#include <cstddef>
#include <string_view>

namespace erbsland::text::pattern {

/// Non-owning UTF-32 text element for static pattern construction.
/// @tested{StringPatternTest}
class Text final {
public:
    /// Create an empty text element.
    constexpr Text() noexcept = default;
    /// Create a text element from a UTF-32 literal.
    template <std::size_t N>
    explicit constexpr Text(const char32_t (&data)[N]) noexcept : _text{data, N - 1U} {}
    /// Create a text element from a UTF-32 read-only string.
    explicit constexpr Text(const std::u32string_view text) noexcept : _text{text} {}

public: // accessors
    /// Access the literal text.
    [[nodiscard]] constexpr auto view() const noexcept -> std::u32string_view { return _text; }

private:
    std::u32string_view _text; ///< The referenced UTF-32 text.
};

}
