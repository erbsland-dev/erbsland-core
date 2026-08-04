// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringKey_fwd.hpp"

#include "../Char.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringEditor_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringEditor_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringEditor_fwd.hpp"

#include <compare>
#include <concepts>
#include <cstddef>
#include <type_traits>

namespace erbsland::text::impl {

#define ERBSLAND_STRING_TYPES_SPECIALIZATION(readOnlyType, editorType)                                                 \
    template <>                                                                                                        \
    struct StringTypes<readOnlyType> {                                                                                 \
        using ReadOnly = readOnlyType;                                                                                 \
        using Editor = editorType;                                                                                     \
    };                                                                                                                 \
    template <>                                                                                                        \
    struct StringTypes<editorType> : StringTypes<readOnlyType> {}

ERBSLAND_STRING_TYPES_SPECIALIZATION(U8String, U8StringEditor);
ERBSLAND_STRING_TYPES_SPECIALIZATION(U16String, U16StringEditor);
ERBSLAND_STRING_TYPES_SPECIALIZATION(U32String, U32StringEditor);

#undef ERBSLAND_STRING_TYPES_SPECIALIZATION

template <typename tValue, typename tString>
concept StringKeyCompatible = std::constructible_from<tString, const tValue &>;

template <typename tString, typename tLeft, typename tRight>
concept HeterogeneousStringKeyPair = StringKeyCompatible<tLeft, tString> && StringKeyCompatible<tRight, tString> &&
    (!std::same_as<std::remove_cvref_t<tLeft>, tString> || !std::same_as<std::remove_cvref_t<tRight>, tString>);

/// Case-sensitive string key ordering.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCompare {
public:
    using is_transparent = void;

    /// Order two strings by their case-sensitive contents.
    /// @param left The left string.
    /// @param right The right string.
    /// @return `true` if `left` precedes `right`.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right) < 0;
    }
    /// Order two string-compatible values by their case-sensitive contents.
    /// @tparam tLeft The left value type.
    /// @tparam tRight The right value type.
    /// @param left The left value.
    /// @param right The right value.
    /// @return `true` if `left` precedes `right`.
    template <typename tLeft, typename tRight>
        requires HeterogeneousStringKeyPair<tString, tLeft, tRight>
    [[nodiscard]] auto operator()(const tLeft &left, const tRight &right) const -> bool {
        return tString{left}.compare(tString{right}) < 0;
    }
};

/// Case-insensitive string key ordering.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCICompare {
public:
    using is_transparent = void;

    /// Order two strings by their case-insensitive contents.
    /// @param left The left string.
    /// @param right The right string.
    /// @return `true` if `left` precedes `right`.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) < 0;
    }
    /// Order two string-compatible values by their case-insensitive contents.
    /// @tparam tLeft The left value type.
    /// @tparam tRight The right value type.
    /// @param left The left value.
    /// @param right The right value.
    /// @return `true` if `left` precedes `right`.
    template <typename tLeft, typename tRight>
        requires HeterogeneousStringKeyPair<tString, tLeft, tRight>
    [[nodiscard]] auto operator()(const tLeft &left, const tRight &right) const -> bool {
        return tString{left}.compare(tString{right}, Char::compareCaseFolded) < 0;
    }
};

/// Case-sensitive string key hash.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringHash {
public:
    using is_transparent = void;

    /// Hash a string using its case-sensitive contents.
    /// @param value The string to hash.
    /// @return The hash value.
    [[nodiscard]] auto operator()(const tString &value) const noexcept -> std::size_t { return value.toHash(); }
    /// Hash a string-compatible value using case-sensitive contents.
    /// @tparam tValue The value type.
    /// @param value The value to hash.
    /// @return The hash value.
    template <typename tValue>
        requires StringKeyCompatible<tValue, tString> && (!std::same_as<std::remove_cvref_t<tValue>, tString>)
    [[nodiscard]] auto operator()(const tValue &value) const -> std::size_t {
        return tString{value}.toHash();
    }
};

/// Case-insensitive string key hash.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCIHash {
public:
    using is_transparent = void;

    /// Hash a string using its case-insensitive contents.
    /// @param value The string to hash.
    /// @return The hash value.
    [[nodiscard]] auto operator()(const tString &value) const noexcept -> std::size_t { return value.toHashCI(); }
    /// Hash a string-compatible value using case-insensitive contents.
    /// @tparam tValue The value type.
    /// @param value The value to hash.
    /// @return The hash value.
    template <typename tValue>
        requires StringKeyCompatible<tValue, tString> && (!std::same_as<std::remove_cvref_t<tValue>, tString>)
    [[nodiscard]] auto operator()(const tValue &value) const -> std::size_t {
        return tString{value}.toHashCI();
    }
};

/// Case-sensitive string key equality.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringEqual {
public:
    using is_transparent = void;

    /// Compare two strings for case-sensitive equality.
    /// @param left The left string.
    /// @param right The right string.
    /// @return `true` if the strings are equal.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left == right;
    }
    /// Compare two string-compatible values for case-sensitive equality.
    /// @tparam tLeft The left value type.
    /// @tparam tRight The right value type.
    /// @param left The left value.
    /// @param right The right value.
    /// @return `true` if the values are equal.
    template <typename tLeft, typename tRight>
        requires HeterogeneousStringKeyPair<tString, tLeft, tRight>
    [[nodiscard]] auto operator()(const tLeft &left, const tRight &right) const -> bool {
        return tString{left} == tString{right};
    }
};

/// Case-insensitive string key equality.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCIEqual {
public:
    using is_transparent = void;

    /// Compare two strings for case-insensitive equality.
    /// @param left The left string.
    /// @param right The right string.
    /// @return `true` if the strings are equal.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
    /// Compare two string-compatible values for case-insensitive equality.
    /// @tparam tLeft The left value type.
    /// @tparam tRight The right value type.
    /// @param left The left value.
    /// @param right The right value.
    /// @return `true` if the values are equal.
    template <typename tLeft, typename tRight>
        requires HeterogeneousStringKeyPair<tString, tLeft, tRight>
    [[nodiscard]] auto operator()(const tLeft &left, const tRight &right) const -> bool {
        return tString{left}.compare(tString{right}, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
};

}
