// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MatchBase.hpp"

#include "../text/u16/U16StringView.hpp"
#include "../util/CoGenerator.hpp"

#include <memory>
#include <vector>

namespace erbsland::re {

class Match16;
/// A shared pointer to a UTF-16 match result.
using Match16Ptr = std::shared_ptr<Match16>;

/// An owning UTF-16 match result.
///
/// The returned views from `content()` are valid for the lifetime of this object.
class Match16 : public MatchBase {
protected:
    using MatchBase::MatchBase;

public:
    ~Match16() override = default;

public:
    /// Get the full content of the match.
    [[nodiscard]] auto content() const -> text::U16StringView;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] auto content(CaptureGroupIndex groupIndex) const -> text::U16StringView;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] auto content(const text::StringView &groupName) const -> text::U16StringView;

protected:
    /// Get a view to the contents of a given group.
    /// If the contents can't be resolved, throw an exception or return an empty view.
    /// @param group The capture group to retrieve the content for.
    [[nodiscard]] virtual auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16StringView = 0;
};

/// A generator returning UTF-16 matches.
using Match16Generator = util::CoGenerator<Match16Ptr>;

/// A list of UTF-16 matches.
using Match16List = std::vector<Match16Ptr>;

}
