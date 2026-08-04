// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/re/Match.hpp>
#include <erbsland/unit/ByteRange.hpp>

using namespace erbsland::re;

/// Test match backed by an owning UTF-8 input string.
/// @notest{Test-only mock.}
class MockStringMatch : public Match {
public:
    /// Create a match backed by `inputString`.
    MockStringMatch(CaptureGroupList captureGroupList, const erbsland::text::String &inputString) :
        Match{std::move(captureGroupList)}, _inputString{inputString} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> erbsland::text::String override {
        return _inputString.slice(
            erbsland::unit::ByteRange{
                erbsland::unit::ByteIndex::fromSizeT(group.begin()),
                erbsland::unit::ByteIndex::fromSizeT(group.end())});
    }

private:
    erbsland::text::String _inputString;
};
