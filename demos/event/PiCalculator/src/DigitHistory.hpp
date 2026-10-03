// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/CpCount.hpp>
#include <erbsland/CpIndex.hpp>
#include <erbsland/CpRange.hpp>
#include <erbsland/ItemIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/OverflowError.hpp>
#include <erbsland/String.hpp>

#include <cstddef>

namespace demo {

/// Retain the complete fractional prefix in owned chunks, accessed only on the main loop.
/// @tested{DigitHistoryTest PiSessionTest}
class DigitHistory final {
    /// One owned history chunk with its prefix offset.
    struct Chunk {
        el::CpIndex begin; ///< First fractional digit's zero-based offset.
        el::String digits; ///< Owned ASCII decimal digits.
    };

public: // accessors
    /// Get the number of retained fractional digits.
    [[nodiscard]] auto count() const noexcept -> el::CpCount { return _count; }
    /// Read a digit range, clipping to the retained prefix.
    /// @param range Zero-based code-point range; infinite length selects the remaining prefix.
    /// @return The requested digits, or an empty string for invalid, empty, or out-of-bounds ranges.
    [[nodiscard]] auto range(el::CpRange range) const -> el::String;

public: // updates
    /// Append a nonempty chunk of ASCII decimal digits on main.
    /// @param digits Independently owned fractional digits.
    /// @throws el::OverflowError If the count cannot be represented.
    void append(el::String digits);

private:
    /// Locate the chunk containing an existing fractional offset.
    [[nodiscard]] auto findChunk(el::CpIndex begin) const -> el::ItemIndex;

private:
    el::List<Chunk> _chunks; ///< Ordered chunks with their starting offsets.
    el::CpCount _count;      ///< Number of fractional digits, excluding "3.".
};

}
