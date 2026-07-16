// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32String_fwd.hpp"
#include "U32StringLiteral_fwd.hpp"
#include "U32StringView_fwd.hpp"

#include "impl/U32StringDataView.hpp"
#include "impl/U32StringReadTools.hpp"

#include "../FormatAs_fwd.hpp"
#include "../StringSide.hpp"

#include "../../mem/UnsafeCharPtr.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/CpRange.hpp"

#include <cstddef>
#include <span>

namespace erbsland::text {

/// A thin wrapper around a UTF-32 string literal.
/// This class provides a safe and efficient way to work with UTF-32 string literals.
/// It allows work with literals that are only copied if a modification is required.
/// @tested{U32StringLiteralTest}
class U32StringLiteral final {
    friend class U32String;
    friend class U32StringView;
    friend constexpr auto impl::createU32StringLiteral(const char32_t *data, std::size_t size) noexcept
        -> U32StringLiteral;
    // format
    template <typename T>
    friend struct FormatAsU32Text;

public:
    /// Create a new U32StringCharLiteral from a UTF-32 string `char32_t` literal.
    template <std::size_t N>
    explicit consteval U32StringLiteral(const char32_t (&data)[N]) noexcept : _charPtr{data}, _size{N - 1} {}

public: // tests
    /// Test if this string is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return impl::U32StringReadTools{dataView()}.isEmpty(); }
    /// Test if this string is valid UTF-32.
    [[nodiscard]] auto isValidUtf32() const noexcept -> bool {
        return impl::U32StringReadTools{dataView()}.isValidUtf32();
    }

public: // accessors
    /// Get the UTF-32 code-unit length of this string.
    [[nodiscard]] auto length() const noexcept -> unit::CpLength {
        return impl::U32StringReadTools{dataView()}.length();
    }
    /// Get the UTF-32 code-unit length of this string.
    [[nodiscard]] auto characterLength() const noexcept -> unit::CpLength { return length(); }
    /// Get the index for one side of the string.
    [[nodiscard]] auto indexAt(const StringSide side) const noexcept -> unit::CpIndex {
        return side == StringSide::Front ? unit::CpIndex::zero() : unit::CpIndex::end(length());
    }

private:
    /// Private constructor, used by the literal operators.
    constexpr U32StringLiteral(const char32_t *data, const std::size_t size) noexcept : _charPtr{data}, _size{size} {}
    /// Get the view to the string data.
    [[nodiscard]] auto dataView() const noexcept -> impl::U32StringDataView {
        return impl::U32StringDataView{std::span{_charPtr, _size}, unit::CpRange::fromSizeT(_size)};
    }

private:
    const char32_t *_charPtr; ///< Reference to a literal char string.
    std::size_t _size;        ///< The size of the referenced literal.
};

}
