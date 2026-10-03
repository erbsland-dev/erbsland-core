// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "StringSplitter_fwd.hpp"

#include "../Char.hpp"
#include "../CharSet.hpp"
#include "../StringSplitMode.hpp"

#include "../../unit/IntegerUnitRange.hpp"
#include "../../unit/ItemCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../util/LoopResult.hpp"
#include "../../util/LoopStatus.hpp"

#include <functional>
#include <utility>

namespace erbsland::text::impl {

/// Sequentially split an owning read-only string into shared slices.
///
/// @tparam tString The width-specific owning read-only string type.
/// @seedoc{/reference/text/matching_and_splitting}
/// @tested{StringSplitterTest}
template <typename tString>
class StringSplitter final {
public:
    using String = tString;
    using Index = decltype(std::declval<String>().findFirstOf(std::declval<const CharSet &>()));
    using Range = unit::IntegerUnitRange<typename Index::Unit>;
    /// A callback that processes one shared string slice and controls iteration.
    using ProcessPartFn = std::function<util::LoopStatus(const String &part)>;
    /// A callback that also receives the zero-based source part index.
    using ProcessPartWithIndexFn = std::function<util::LoopStatus(const String &part, unit::ItemIndex index)>;

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
    [[nodiscard]] auto isAtEnd() const noexcept -> bool { return _position.isNoIndex(); }
    /// Get the number of splits made so far.
    [[nodiscard]] auto splitCount() const noexcept -> unit::ItemCount { return _splitCount; }
    /// Get the index of the *next* split.
    /// This is also the index of the *current* part before reading it.
    [[nodiscard]] auto splitIndex() const noexcept -> unit::ItemIndex { return unit::ItemIndex::end(_splitCount); }
    /// Access the complete source text.
    [[nodiscard]] auto text() const noexcept -> const String & { return _text; }
    /// Return the unread suffix as a shared slice.
    [[nodiscard]] auto remaining() const noexcept -> String;

public: // sequential processing
    /// Read the next part as a shared slice.
    /// Calling this method at the end returns an empty string.
    /// @return The next part, optionally including its separator.
    [[nodiscard]] auto next() noexcept -> String;
    /// Skip the next part without creating a shared slice.
    /// Calling this method at the end has no effect.
    void skip() noexcept;
    /// Process the remaining parts, consuming each part before calling the function.
    /// A null function succeeds without consuming any parts. Callback exceptions propagate.
    /// @param function The callback returning Continue, Stop, or Error.
    /// @return Success at the end, Stopped for Stop, or Error for Error.
    auto forEach(const ProcessPartFn &function) -> util::LoopResult;
    /// Process the remaining parts with their zero-based source part indexes.
    /// Indexes continue from splitIndex(). Consumption and loop results match the part-only overload.
    /// @param function The callback receiving the part and its ItemIndex.
    /// @return Success at the end, Stopped for Stop, or Error for Error.
    auto forEach(const ProcessPartWithIndexFn &function) -> util::LoopResult;
    /// Restart splitting at the beginning of the source text.
    void reset() noexcept;

private:
    /// Advance to the next part and optionally store its source range.
    /// @param partRange Where to store the consumed part range, or `nullptr` if no range is needed.
    void advanceToNextPart(Range *partRange) noexcept;

private:
    String _text;                ///< The owning source text.
    CharSet _separators;         ///< The separator characters.
    Index _position;             ///< The start of the next part, or noIndex() when all parts were read.
    unit::ItemCount _splitCount; ///< A split counter to keep tracks of the current part number.
    StringSplitMode _mode{StringSplitMode::DiscardSeparator}; ///< How separators are returned.
};

}

#include "StringSplitter.tpp"
