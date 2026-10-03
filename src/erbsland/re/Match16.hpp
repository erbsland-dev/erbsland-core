// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Match16_fwd.hpp"
#include "MatchBase.hpp"

#include "../text/u16/U16String_fwd.hpp"
#include "../util/CoGenerator.hpp"

namespace erbsland::re {

/// An owning UTF-16 match result.
///
/// The returned views from `content()` are valid for the lifetime of this object.
class Match16 : public MatchBase {
protected:
    using MatchBase::MatchBase;

public:
    // defaults
    ~Match16() override = default;

public:
    /// Get the full content of the match.
    [[nodiscard]] auto content() const -> text::U16String;
    /// Get the content of the specified group.
    /// @param groupIndex The capture-group index. Must identify a group in this match.
    [[nodiscard]] auto content(CaptureGroupIndex groupIndex) const -> text::U16String;
    /// Get the content of the specified group.
    /// @param groupName The capture-group name. Must identify a named group in this match.
    [[nodiscard]] auto content(const text::String &groupName) const -> text::U16String;

protected:
    /// Get a view to the contents of a given group.
    /// If the contents can't be resolved, throw an exception or return an empty view.
    /// @param group The capture group to retrieve the content for.
    [[nodiscard]] virtual auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16String = 0;
};

}
