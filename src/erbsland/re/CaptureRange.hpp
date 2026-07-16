// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "InputPosition.hpp"

#include "../text/StdFormatForText.hpp"
#include "../text/StringFormat.hpp"

#include <format>

namespace erbsland::re {

/// Represents a range in the input, defined by begin and end positions.
///
/// This class is used to represent captured content ranges in regular expression matches.
/// The range is defined as [begin, end], where begin is inclusive and end is exclusive.
///
class CaptureRange {
public:
    /// Create a new capture range with the given begin and end positions.
    /// @param begin The start position of the range (inclusive).
    /// @param end The end position of the range (exclusive).
    constexpr CaptureRange(const InputPosition begin, const InputPosition end) noexcept : _begin{begin}, _end{end} {}

    // defaults
    CaptureRange() = default;
    ~CaptureRange() = default;
    CaptureRange(const CaptureRange &) = default;
    auto operator=(const CaptureRange &) noexcept -> CaptureRange & = default;
    auto operator==(const CaptureRange &) const noexcept -> bool = default;
    auto operator!=(const CaptureRange &) const noexcept -> bool = default;

public: // accessors
    /// Test if the range is empty.
    /// @return True if begin equals end, false otherwise.
    [[nodiscard]] constexpr auto isEmpty() const noexcept -> bool { return _begin == _end; }
    /// Get the size of the range.
    /// @return The number of positions between begin and end.
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t { return _end - _begin; }
    /// Get the start position of the range.
    /// @return The begin position (inclusive).
    [[nodiscard]] constexpr auto begin() const noexcept -> InputPosition { return _begin; }
    /// Get the end position of the range.
    /// @return The end position (exclusive).
    [[nodiscard]] constexpr auto end() const noexcept -> InputPosition { return _end; }

public: // modifiers
    /// Set the start position of the range.
    /// @param begin The new begin position (inclusive).
    void setBegin(const InputPosition begin) noexcept { _begin = begin; }
    /// Set the end position of the range.
    /// @param end The new end position (exclusive).
    void setEnd(const InputPosition end) noexcept { _end = end; }

public:
    /// Convert the range into a short, human-readable string.
    /// @return The formatted range as `"begin-end"`.
    [[nodiscard]] auto toString() const -> text::String { return text::StringFormat{"{}-{}"}.build(_begin, _end); }

private:
    InputPosition _begin;
    InputPosition _end;
};

}

template <>
struct std::formatter<erbsland::re::CaptureRange> : std::formatter<erbsland::text::String> {
    auto format(const erbsland::re::CaptureRange &range, std::format_context &ctx) const {
        return std::formatter<erbsland::text::String>::format(range.toString(), ctx);
    }
};
