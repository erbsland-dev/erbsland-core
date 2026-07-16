// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match16.hpp"

namespace erbsland::re::impl {

class U16StringMatch final : public Match16 {
public:
    U16StringMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList, const text::U16StringView &text) :
        Match16{std::move(regEx), std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16StringView override;

private:
    text::U16StringView _text;
};

}
