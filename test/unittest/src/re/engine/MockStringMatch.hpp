// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/re/Match.hpp>
#include <erbsland/unit/ByteRange.hpp>

using namespace erbsland::re;

class MockStringMatch : public Match {
public:
    MockStringMatch(
        ConstRegExPtr regEx, CaptureGroupList captureGroupList, const erbsland::text::StringView &inputStringView) :
        Match{std::move(regEx), std::move(captureGroupList)}, _inputStringView{inputStringView} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept
        -> erbsland::text::StringView override {
        return _inputStringView.slice(
            erbsland::unit::ByteRange{
                erbsland::unit::ByteIndex::fromSizeT(group.begin()),
                erbsland::unit::ByteIndex::fromSizeT(group.end())});
    }

private:
    erbsland::text::StringView _inputStringView;
};
