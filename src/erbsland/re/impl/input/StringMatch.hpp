// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match.hpp"

namespace erbsland::re::impl {

/// UTF-8 capture-group match backed by an owning string.
class StringMatch final : public Match {
public:
    /// Create a match backed by `text`.
    StringMatch(CaptureGroupList captureGroupList, const text::String &text) :
        Match{std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::String override;

private:
    text::String _text;
};

}
