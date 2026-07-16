// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "MatchBase.hpp"

#include "../text/u32/U32StringView.hpp"
#include "../util/CoGenerator.hpp"

#include <memory>
#include <vector>

namespace erbsland::re {

class Match32;
/// A shared pointer to a UTF-32 match result.
using Match32Ptr = std::shared_ptr<Match32>;

/// An owning UTF-32 match result.
///
/// The returned views from `content()` are valid for the lifetime of this object.
class Match32 : public MatchBase {
protected:
    using MatchBase::MatchBase;

public:
    ~Match32() override = default;

public:
    /// Get the full content of the match.
    [[nodiscard]] auto content() const -> text::U32StringView;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] auto content(CaptureGroupIndex groupIndex) const -> text::U32StringView;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] auto content(const text::StringView &groupName) const -> text::U32StringView;

protected:
    /// Get a view to the contents of a given group.
    /// If the contents can't be resolved, throw an exception or return an empty view.
    /// @param group The capture group to retrieve the content for.
    [[nodiscard]] virtual auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U32StringView = 0;
};

/// A generator returning UTF-32 matches.
using Match32Generator = util::CoGenerator<Match32Ptr>;

/// A list of UTF-32 matches.
using Match32List = std::vector<Match32Ptr>;

}
