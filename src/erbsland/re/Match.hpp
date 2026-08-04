// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Match_fwd.hpp"
#include "MatchBase.hpp"

#include "../text/String_fwd.hpp"
#include "../util/CoGenerator.hpp"

namespace erbsland::re {

/// An owning match result.
///
/// The returned views from `content()` are valid for the lifetime of this object.
class Match : public MatchBase {
protected:
    using MatchBase::MatchBase;

public:
    // defaults
    ~Match() override = default;

public:
    /// Get the full content of the match.
    [[nodiscard]] auto content() const -> text::String;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group index is invalid.
    [[nodiscard]] auto content(CaptureGroupIndex groupIndex) const -> text::String;
    /// Get the content of the specified group.
    /// @throws err::ParameterError if the group name is invalid.
    [[nodiscard]] auto content(const text::String &groupName) const -> text::String;

protected:
    /// Get a view to the contents of a given group.
    /// If the contents can't be resolved, throw an exception or return an empty view.
    /// @param group The capture group to retrieve the content for.
    [[nodiscard]] virtual auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::String = 0;
};

}
