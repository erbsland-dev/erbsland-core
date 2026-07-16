// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../../unit/U16DataRange.hpp"

namespace erbsland::re::impl {

auto U16StringMatch::getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16StringView {
    const auto beginIndex = unit::U16DataIndex::fromSizeT(group.begin());
    const auto endIndex = unit::U16DataIndex::fromSizeT(group.end());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(beginIndex <= endIndex, "Capture group begin index must not exceed end index"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        endIndex <= unit::U16DataIndex::end(_text.length()), "Capture group end index out of bounds"_el);
    return _text.slice(unit::U16DataRange{beginIndex, endIndex});
}

}
