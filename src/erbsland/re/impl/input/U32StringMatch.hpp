// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../Match32.hpp"

namespace erbsland::re::impl {

class U32StringMatch final : public Match32 {
public:
    U32StringMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList, const text::U32StringView &text) :
        Match32{std::move(regEx), std::move(captureGroupList)}, _text{text} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::U32StringView override;

private:
    text::U32StringView _text;
};

}
