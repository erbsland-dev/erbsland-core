// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match16.hpp"

namespace erbsland::re::impl {

/// UTF-16 capture-group match backed by an owning string.
class U16StringMatch final : public Match16 {
public:
    /// Create a match backed by `text`.
    U16StringMatch(CaptureGroupList captureGroupList, const text::U16String &text) :
        Match16{std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16String override;

private:
    text::U16String _text;
};

}
