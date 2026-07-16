// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroup.hpp"
#include "InputPosition.hpp"

#include "../text/StringCIHashMap.hpp"
#include "../text/StringView.hpp"

#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace erbsland::re {

class RegEx;
/// A shared pointer to an immutable regular expression.
using ConstRegExPtr = std::shared_ptr<const RegEx>;

class MatchBase;
/// A shared pointer to a match base result.
using MatchBasePtr = std::shared_ptr<MatchBase>;

/// The abstract baseclass for regular expression matches.
class MatchBase {
public:
    virtual ~MatchBase() = default;
    MatchBase(const MatchBase &) = delete;
    auto operator=(const MatchBase &) -> MatchBase & = delete;
    MatchBase(MatchBase &&) = delete;
    auto operator=(MatchBase &&) -> MatchBase & = delete;

public:
    /// Get the start position of the match.
    [[nodiscard]] virtual auto begin() const -> InputPosition;
    /// Get the start position of the given group.
    /// @param groupIndex The index of the group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] virtual auto begin(CaptureGroupIndex groupIndex) const -> InputPosition;
    /// Get the start position of the given group.
    /// @param groupName The name of the group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] virtual auto begin(const text::StringView &groupName) const -> InputPosition;

    /// Get the end position of the match.
    /// The end position points *after* the last character of the match.
    [[nodiscard]] virtual auto end() const -> InputPosition;
    /// Get the end position of the given group.
    /// The end position points *after* the last character of the match.
    /// @param groupIndex The index of the group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] virtual auto end(CaptureGroupIndex groupIndex) const -> InputPosition;
    /// Get the end position of the given group.
    /// The end position points *after* the last character of the match.
    /// @param groupName The name of the group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] virtual auto end(const text::StringView &groupName) const -> InputPosition;

    /// Get the range of the match.
    /// The range is defined as [begin, end] where begin is inclusive and end is exclusive.
    [[nodiscard]] virtual auto range() const -> CaptureRange;
    /// Get the range of the given group.
    /// The range is defined as [begin, end] where begin is inclusive and end is exclusive.
    /// @param groupIndex The index of the group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] virtual auto range(CaptureGroupIndex groupIndex) const -> CaptureRange;
    /// Get the range of the given group.
    /// The range is defined as [begin, end] where begin is inclusive and end is exclusive.
    /// @param groupName The group name.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] virtual auto range(const text::StringView &groupName) const -> CaptureRange;

    /// Get the capture group of the match.
    /// @return A reference to the capture group instance.
    [[nodiscard]] virtual auto group() const -> const CaptureGroup &;
    /// Get the capture group of the given group.
    /// @param groupIndex The index of the group.
    /// @return A reference to the capture group instance.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] virtual auto group(CaptureGroupIndex groupIndex) const -> const CaptureGroup &;
    /// Get the capture group of the given group.
    /// @param groupName The name of the group.
    /// @return A reference to the capture group instance.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] virtual auto group(const text::StringView &groupName) const -> const CaptureGroup &;

    /// Get the number of capture groups, including the full match.
    /// If there are two capture groups `(a)(b)`, this function will return 3, as the full match counts as
    /// capture group zero `((a)(b))`.
    [[nodiscard]] virtual auto groupCount() const noexcept -> std::size_t;
    /// Test if the given group index exists.
    [[nodiscard]] virtual auto hasGroupIndex(CaptureGroupIndex groupIndex) const noexcept -> bool;
    /// Test if the given group name exists.
    [[nodiscard]] virtual auto hasGroupName(const text::StringView &groupName) const noexcept -> bool;

protected:
    /// Create a new match object.
    /// @param regEx The regular expression object that created the match.
    /// @param groups The capture groups of the match (zero = full match, 1 = first capture group).
    MatchBase(ConstRegExPtr regEx, std::vector<CaptureGroup> groups) noexcept;

protected:
    /// Get the index of a group by name.
    /// This will build the group name to an index map on the first call.
    /// @param groupName The group name.
    /// @return The index of the group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] auto getGroupIndex(const text::StringView &groupName) const -> CaptureGroupIndex;

    /// Build the group name to index map.
    void buildGroupNameToGroupIndexMap() const noexcept;

protected:
    ConstRegExPtr _regEx;     ///< A pointer to the regular expression that created this match.
    CaptureGroupList _groups; ///< The capture-groups of the match.

private:
    /// A flag to indicate if the group name to the index map has been initialized.
    mutable std::once_flag _groupNameToGroupIndexMapInitFlag;
    /// A cache to resolve group names to group indexes.
    /// This cache only stores views to the names, it is linked to the lifetime of `_regEx`.
    mutable std::optional<text::StringCIHashMap<CaptureGroupIndex>> _nameToGroupIndexMap;
};

}
