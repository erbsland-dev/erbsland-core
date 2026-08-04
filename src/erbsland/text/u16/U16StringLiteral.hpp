// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16String_fwd.hpp"
#include "U16StringEditor_fwd.hpp"
#include "U16StringLiteral_fwd.hpp"

#include "impl/U16StringCharReadTool.hpp"
#include "impl/U16StringDataView.hpp"
#include "impl/U16StringReadTools.hpp"

#include "../StringSide.hpp"

#include "../../mem/UnsafeCharPtr.hpp"
#include "../../unit/U16DataLength.hpp"
#include "../../unit/U16DataRange.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text {

/// A thin wrapper around a UTF-16 string literal.
/// This class provides a safe and efficient way to work with UTF-16 string literals.
/// It allows work with literals that are only copied if a modification is required.
/// @tested{U16StringLiteralTest}
class U16StringLiteral final {
    friend class U16StringEditor;
    friend class U16String;
    friend constexpr auto impl::createU16StringLiteral(const char16_t *data, std::size_t size) noexcept
        -> U16StringLiteral;

public:
    /// Create a new U16StringCharLiteral from a UTF-16 string `char16_t` literal.
    template <std::size_t N>
    explicit consteval U16StringLiteral(const char16_t (&data)[N]) noexcept : _charPtr{data}, _size{N - 1} {}

public: // tests
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return impl::U16StringReadTools{dataView()}.isEmpty(); }
    /// Test if this string is valid UTF-16.
    [[nodiscard]] auto isValidUtf16() const noexcept -> bool {
        return impl::U16StringReadTools{dataView()}.isValidUtf16();
    }

public: // accessors
    /// Get the UTF-16 code-unit length of this string.
    [[nodiscard]] auto length() const noexcept -> unit::U16DataLength {
        return impl::U16StringReadTools{dataView()}.byteLength();
    }
    /// Get the code-point length of this string.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength {
        return impl::U16StringCharReadTool{dataView()}.charLength();
    }
    /// Get the index for one side of the string.
    [[nodiscard]] auto indexAt(const StringSide side) const noexcept -> unit::U16DataIndex {
        return side == StringSide::Front ? unit::U16DataIndex::zero() : unit::U16DataIndex::end(length());
    }

private:
    /// Private constructor, used by the literal operators.
    constexpr U16StringLiteral(const char16_t *data, const std::size_t size) noexcept : _charPtr{data}, _size{size} {}
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U16StringDataView {
        return impl::U16StringDataView{std::span{_charPtr, _size}, unit::U16DataRange::fromSizeT(_size)};
    }

private:
    const char16_t *_charPtr; ///< Reference to a literal char string.
    std::size_t _size;        ///< The size of the referenced literal.
};

}
