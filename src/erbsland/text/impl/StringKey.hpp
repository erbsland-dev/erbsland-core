// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Char.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u16/U16StringView_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u32/U32StringView_fwd.hpp"
#include "../u8/U8String_fwd.hpp"
#include "../u8/U8StringView_fwd.hpp"

#include <compare>
#include <cstddef>

namespace erbsland::text::impl {

template <typename tString>
struct StringViewFor;

template <>
struct StringViewFor<U8String> {
    using Type = U8StringView;
};

template <>
struct StringViewFor<U16String> {
    using Type = U16StringView;
};

template <>
struct StringViewFor<U32String> {
    using Type = U32StringView;
};

/// Case-sensitive string key ordering.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCompare {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Test if the left string is ordered before the right string.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right) < 0;
    }
    /// Test if the left string is ordered before the right string.
    [[nodiscard]] auto operator()(const tString &left, const View &right) const noexcept -> bool {
        return left.compare(right) < 0;
    }
    /// Test if the left string is ordered before the right string.
    [[nodiscard]] auto operator()(const View &left, const tString &right) const noexcept -> bool {
        return left.compare(right) < 0;
    }
    /// Test if the left string is ordered before the right string.
    [[nodiscard]] auto operator()(const View &left, const View &right) const noexcept -> bool {
        return left.compare(right) < 0;
    }
};

/// Case-insensitive string key ordering.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCICompare {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Test if the left string is ordered before the right string using Unicode simple case folding.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) < 0;
    }
    /// Test if the left string is ordered before the right string using Unicode simple case folding.
    [[nodiscard]] auto operator()(const tString &left, const View &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) < 0;
    }
    /// Test if the left string is ordered before the right string using Unicode simple case folding.
    [[nodiscard]] auto operator()(const View &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) < 0;
    }
    /// Test if the left string is ordered before the right string using Unicode simple case folding.
    [[nodiscard]] auto operator()(const View &left, const View &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) < 0;
    }
};

/// Case-sensitive string key hash.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringHash {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Hash the string key.
    [[nodiscard]] auto operator()(const tString &value) const noexcept -> std::size_t { return value.toHash(); }
    /// Hash the string key.
    [[nodiscard]] auto operator()(const View &value) const noexcept -> std::size_t { return value.toHash(); }
};

/// Case-insensitive string key hash.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCIHash {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Hash the string key using Unicode simple case folding.
    [[nodiscard]] auto operator()(const tString &value) const noexcept -> std::size_t { return value.toHashCI(); }
    /// Hash the string key using Unicode simple case folding.
    [[nodiscard]] auto operator()(const View &value) const noexcept -> std::size_t { return value.toHashCI(); }
};

/// Case-sensitive string key equality.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringEqual {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Test if the string keys are equal.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left == right;
    }
    /// Test if the string keys are equal.
    [[nodiscard]] auto operator()(const tString &left, const View &right) const noexcept -> bool {
        return left == right;
    }
    /// Test if the string keys are equal.
    [[nodiscard]] auto operator()(const View &left, const tString &right) const noexcept -> bool {
        return left == right;
    }
    /// Test if the string keys are equal.
    [[nodiscard]] auto operator()(const View &left, const View &right) const noexcept -> bool { return left == right; }
};

/// Case-insensitive string key equality.
/// @tested{StringMapTest StringSetTest}
template <typename tString>
class StringCIEqual {
public:
    using is_transparent = void;
    using View = typename StringViewFor<tString>::Type;

    /// Test if the string keys are equal using Unicode simple case folding.
    [[nodiscard]] auto operator()(const tString &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
    /// Test if the string keys are equal using Unicode simple case folding.
    [[nodiscard]] auto operator()(const tString &left, const View &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
    /// Test if the string keys are equal using Unicode simple case folding.
    [[nodiscard]] auto operator()(const View &left, const tString &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
    /// Test if the string keys are equal using Unicode simple case folding.
    [[nodiscard]] auto operator()(const View &left, const View &right) const noexcept -> bool {
        return left.compare(right, Char::compareCaseFolded) == std::strong_ordering::equal;
    }
};

}
