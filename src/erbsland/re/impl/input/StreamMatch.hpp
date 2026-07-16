// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../text/String.hpp"
#include "../../Match.hpp"

#include <vector>

namespace erbsland::re::impl {

/// A match with capture content copied from a text stream.
/// @tested{RegExStreamInputTest}
class StreamMatch final : public Match {
public:
    StreamMatch(ConstRegExPtr regEx, CaptureGroupList captureGroupList, std::vector<text::String> content) :
        Match{std::move(regEx), std::move(captureGroupList)}, _content{std::move(content)} {}

protected:
    [[nodiscard]] auto getContentForGroup(const CaptureGroup &group) const noexcept -> text::StringView override;

private:
    std::vector<text::String> _content; ///< Copied content by capture group index.
};

}
