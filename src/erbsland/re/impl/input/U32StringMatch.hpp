// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match32.hpp"

namespace erbsland::re::impl {

/// UTF-32 capture-group match backed by an owning string.
class U32StringMatch final : public Match32 {
public:
    /// Create a match backed by `text`.
    U32StringMatch(CaptureGroupList captureGroupList, const text::U32String &text) :
        Match32{std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U32String override;

private:
    text::U32String _text;
};

}
