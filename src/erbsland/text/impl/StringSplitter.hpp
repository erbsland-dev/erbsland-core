// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringSplitter_fwd.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../StringSplitMode.hpp"

#include "../../unit/IntegerUnitRange.hpp"

#include <utility>

namespace erbsland::text::impl {

/// Sequentially split an owning read-only string into shared slices.
///
/// @tparam tString The width-specific owning read-only string type.
/// @seedoc{/reference/text/string_splitter}
/// @tested{StringSplitterTest}
template <typename tString>
class StringSplitter final {
public:
    using String = tString;
    using Index = decltype(std::declval<String>().findFirstOf(std::declval<const CharSet &>()));
    using Range = unit::IntegerUnitRange<typename Index::Unit>;

public:
    /// Create a splitter for one separator character.
    /// @param text The owning text to split.
    /// @param separator The separator character.
    /// @param mode Whether returned parts keep their trailing separator.
    explicit StringSplitter(
        String text, Char separator, StringSplitMode mode = StringSplitMode::DiscardSeparator) noexcept;
    /// Create a splitter for a set of separator characters.
    /// @param text The owning text to split.
    /// @param separators The separator characters.
    /// @param mode Whether returned parts keep their trailing separator.
    explicit StringSplitter(
        String text, CharSet separators, StringSplitMode mode = StringSplitMode::DiscardSeparator) noexcept;

    // defaults
    ~StringSplitter() = default;
    StringSplitter(const StringSplitter &) noexcept = default;
    StringSplitter(StringSplitter &&) noexcept = default;
    auto operator=(const StringSplitter &) noexcept -> StringSplitter & = default;
    auto operator=(StringSplitter &&) noexcept -> StringSplitter & = default;

public: // accessors
    /// Test if all parts were read.
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _isAtEnd; }
    /// Access the complete source text.
    [[nodiscard]] auto text() const noexcept -> const String & { return _text; }
    /// Return the unread suffix as a shared slice.
    [[nodiscard]] auto remaining() const noexcept -> String;

public:
    /// Read the next part as a shared slice.
    /// Calling this method at the end returns an empty string.
    /// @return The next part, optionally including its separator.
    [[nodiscard]] auto next() noexcept -> String;
    /// Restart splitting at the beginning of the source text.
    void reset() noexcept;

private:
    String _text;                                             ///< The owning source text.
    CharSet _separators;                                      ///< The separator characters.
    Index _position;                                          ///< The start of the next part.
    StringSplitMode _mode{StringSplitMode::DiscardSeparator}; ///< How separators are returned.
    bool _isAtEnd{false};                                     ///< If all parts were read.
};

}

#include "StringSplitter.tpp"
