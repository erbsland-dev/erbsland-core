// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupIndex.hpp"
#include "CaptureRange.hpp"
#include "InputPosition.hpp"

#include "../text/StringView.hpp"

#include <cstdint>
#include <vector>

namespace erbsland::re {

/// The definition of a match group.
class CaptureGroup {
public:
    /// Create an empty capture group.
    constexpr CaptureGroup() noexcept = default;

    /// Create a new match group with the given capture range, and name.
    /// @param index The index of the group. 0 = complete match, 1 = first capture group.
    /// @param range The character range for the group.
    /// @param name The name of the group.
    constexpr CaptureGroup(
        const CaptureGroupIndex index, const CaptureRange range, const text::StringView &name) noexcept :
        _index{index}, _range{range}, _name{name} {}

public: // accessors
    /// Get the index of this capture group.
    [[nodiscard]] constexpr auto index() const noexcept -> CaptureGroupIndex { return _index; }
    /// Test if the group is empty.
    /// @return True if begin equals end, false otherwise.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _range.isEmpty(); }
    /// Get the size of the group.
    /// @return The number of positions between begin and end.
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return _range.size(); }
    /// Get the start position for the match.
    /// @return The begin position (inclusive).
    [[nodiscard]] constexpr auto begin() const noexcept -> InputPosition { return _range.begin(); }
    /// Get the end position for the match.
    /// @return The end position (exclusive).
    [[nodiscard]] constexpr auto end() const noexcept -> InputPosition { return _range.end(); }
    /// Get the range of for the match.
    /// @return The range.
    [[nodiscard]] constexpr auto range() const noexcept -> CaptureRange { return _range; }
    /// Get the name of the match group.
    [[nodiscard]] constexpr auto name() const noexcept -> text::StringView { return _name; }

public: // modifiers
    /// Set the index for the group.
    /// @param index The group index.
    void setIndex(const CaptureGroupIndex index) noexcept { _index = index; }
    /// Set the start position for the match.
    /// @param begin The begin position (inclusive).
    void setBegin(const InputPosition begin) noexcept { _range.setBegin(begin); }
    /// Set the end position for the match.
    /// @param end The end position (exclusive).
    void setEnd(const InputPosition end) noexcept { _range.setEnd(end); }
    /// Set the name of the match group.
    /// @param name The name of the group.
    void setName(const text::StringView &name) noexcept { _name = name; }

private:
    CaptureGroupIndex _index{};
    CaptureRange _range{};
    text::StringView _name;
};

/// A list of capture groups.
using CaptureGroupList = std::vector<CaptureGroup>;

}
