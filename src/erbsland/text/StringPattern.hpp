// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringPattern_fwd.hpp"

#include "pattern/AnyElement.hpp"
#include "u16/U16String.hpp"
#include "u16/U16StringView.hpp"
#include "u32/U32String.hpp"
#include "u32/U32StringView.hpp"
#include "u8/U8String.hpp"
#include "u8/U8StringView.hpp"

#include "../unit/ByteIndex.hpp"
#include "../unit/ByteLength.hpp"
#include "../unit/ByteRange.hpp"
#include "../unit/CpIndex.hpp"
#include "../unit/CpLength.hpp"
#include "../unit/CpRange.hpp"
#include "../unit/U16DataIndex.hpp"
#include "../unit/U16DataLength.hpp"
#include "../unit/U16DataRange.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace erbsland::text {

namespace impl {

class StringPatternData;

}

/// A lightweight decoded-character string pattern.
/// @seedoc{/reference/text/string_pattern}
/// @tested{StringPatternTest}
class StringPattern final {
public:
    /// Maximum number of elements in a typed pattern.
    static constexpr auto cMaximumElements = pattern::cMaximumElements;
    /// Maximum number of total ranges in a typed pattern.
    static constexpr auto cMaximumRanges = pattern::cMaximumStaticRanges;
    /// Maximum number of ranges in one set element.
    static constexpr auto cMaximumSetRanges = pattern::cMaximumSetRanges;

public:
    /// Create a pattern that never matches.
    StringPattern() noexcept = default;
    /// Parse a pattern from a UTF-8 string.
    explicit StringPattern(const U8StringView &pattern);
    /// Parse a pattern from a UTF-16 string.
    explicit StringPattern(const U16StringView &pattern);
    /// Parse a pattern from a UTF-32 string.
    explicit StringPattern(const U32StringView &pattern);
    /// Create a typed pattern from pattern elements.
    template <pattern::AnyElement... Args>
    explicit StringPattern(const Args &...elements);
    // defaults
    ~StringPattern() = default;
    StringPattern(const StringPattern &) noexcept = default;
    StringPattern(StringPattern &&) noexcept = default;
    auto operator=(const StringPattern &) noexcept -> StringPattern & = default;
    auto operator=(StringPattern &&) noexcept -> StringPattern & = default;

public:
    /// Test if this pattern matches the front or divided front/back parts of the text.
    [[nodiscard]] auto matches(const U8StringView &text) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto matches(const U16StringView &text) const noexcept -> bool;
    /// @overload
    [[nodiscard]] auto matches(const U32StringView &text) const noexcept -> bool;
    /// Trim the matching part from the view if the pattern matches.
    auto trim(U8StringView &text) const noexcept -> bool;
    /// @overload
    auto trim(U16StringView &text) const noexcept -> bool;
    /// @overload
    auto trim(U32StringView &text) const noexcept -> bool;
    /// Trim the matching part from the string if the pattern matches.
    auto trim(U8String &text) const -> bool;
    /// @overload
    auto trim(U16String &text) const -> bool;
    /// @overload
    auto trim(U32String &text) const -> bool;
    /// Return the trimmed view, or the original view if there is no match.
    [[nodiscard]] auto trimmed(const U8StringView &text) const noexcept -> U8StringView;
    /// @overload
    [[nodiscard]] auto trimmed(const U16StringView &text) const noexcept -> U16StringView;
    /// @overload
    [[nodiscard]] auto trimmed(const U32StringView &text) const noexcept -> U32StringView;
    /// Split the text at the matching pattern boundary.
    [[nodiscard]] auto split(const U8StringView &text) const noexcept -> std::pair<U8StringView, U8StringView>;
    /// @overload
    [[nodiscard]] auto split(const U16StringView &text) const noexcept -> std::pair<U16StringView, U16StringView>;
    /// @overload
    [[nodiscard]] auto split(const U32StringView &text) const noexcept -> std::pair<U32StringView, U32StringView>;
    /// Return the native length of the matching text, or zero if there is no match.
    [[nodiscard]] auto length(const U8StringView &text) const noexcept -> unit::ByteLength;
    /// @overload
    [[nodiscard]] auto length(const U16StringView &text) const noexcept -> unit::U16DataLength;
    /// @overload
    [[nodiscard]] auto length(const U32StringView &text) const noexcept -> unit::CpLength;
    /// Return the native split index, or no-index if there is no match.
    [[nodiscard]] auto index(const U8StringView &text) const noexcept -> unit::ByteIndex;
    /// @overload
    [[nodiscard]] auto index(const U16StringView &text) const noexcept -> unit::U16DataIndex;
    /// @overload
    [[nodiscard]] auto index(const U32StringView &text) const noexcept -> unit::CpIndex;

private:
    std::shared_ptr<const impl::StringPatternData> _data; ///< The immutable compiled pattern data.
};

}

#include "StringPattern.tpp"
