// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match.hpp"

namespace erbsland::re::impl {

class StringMatch final : public Match {
public:
    StringMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList, const text::StringView &text) :
        Match{std::move(regEx), std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::StringView override;

private:
    text::StringView _text;
};

}
