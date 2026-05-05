// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AbbreviationListOffset.hpp"
#include "AbbreviationOffset.hpp"

#include "../../TimeAmounts.hpp"

#include <cstddef>
#include <cstdint>

namespace erbsland::time::tz::impl {

/// One generated continuation line for a time-zone definition.
/// @notest{Internal generated-data helper.}
struct ContinuationLine final {
    [[nodiscard]] constexpr auto isRuleSetBased() const noexcept -> bool { return offsetOrRuleSetIndex >= 0x10000; }
    [[nodiscard]] constexpr auto ruleSetIndex() const noexcept -> std::size_t {
        return static_cast<std::size_t>(offsetOrRuleSetIndex & 0x0000'000f);
    }
    [[nodiscard]] constexpr auto abbreviationListOffset() const noexcept -> AbbreviationListOffset {
        return static_cast<AbbreviationListOffset>((offsetOrRuleSetIndex & 0x0000'fff0) >> 4);
    }
    [[nodiscard]] auto fixedOffset() const noexcept -> Seconds { return Seconds{offsetOrRuleSetIndex}; }

    int32_t standardOffset : 18;
    uint64_t until : 40;
    int32_t untilOffset : 18;
    int32_t offsetOrRuleSetIndex : 18;
    AbbreviationOffset abbreviationOffset : 5;
};

}
