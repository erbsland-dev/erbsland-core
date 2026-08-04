// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8String_fwd.hpp"
#include "U8StringEditor_fwd.hpp"
#include "U8StringLiteral_fwd.hpp"

#include "impl/U8StringDataView.hpp"

#include "../StringSide.hpp"

#include "../../unit/ByteLength.hpp"
#include "../../unit/ByteRange.hpp"
#include "../../unit/CpLength.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text {

/// A thin wrapper around a UTF-8 string literal.
/// This class provides a safe and efficient way to work with UTF-8 string literals.
/// It allows work with literals that are only copied if a modification is required.
/// @tested{U8StringLiteralTest}
template <typename tChar>
class U8StringLiteral final {
    friend class U8StringEditor;
    friend class U8String;
    friend constexpr auto impl::createU8StringLiteral(const char *data, std::size_t size) noexcept
        -> U8StringLiteral<char>;
    friend constexpr auto impl::createU8StringLiteral(const char8_t *data, std::size_t size) noexcept
        -> U8StringLiteral<char8_t>;

public:
    /// Create a new U8StringCharLiteral from a UTF-8 string `char` literal.
    template <std::size_t N>
    explicit consteval U8StringLiteral(const tChar (&data)[N]) noexcept : _charPtr{data}, _size{N - 1} {}

public: // tests
    /// Test if this string is empty.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool;
    /// Test if this string is valid UTF-8.
    [[nodiscard]] auto isValidUtf8() const noexcept -> bool;

public: // accessors
    /// Get the byte length of this string.
    [[nodiscard]] constexpr auto length() const noexcept -> unit::ByteLength;
    /// Get the code-point length of this string.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength;
    /// Get the native data index for one side of the string.
    [[nodiscard]] constexpr auto indexAt(StringSide side) const noexcept -> unit::ByteIndex;

private:
    /// Private constructor, used by the literal operators.
    constexpr U8StringLiteral(const tChar *data, const std::size_t size) noexcept : _charPtr{data}, _size{size} {}
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U8StringDataView;

private:
    const tChar *_charPtr; ///< Reference to a literal char string.
    std::size_t _size;     ///< The size of the referenced literal.
};

}

#include "U8StringLiteral.tpp"
