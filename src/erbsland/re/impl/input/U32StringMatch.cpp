// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U32StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../../unit/CpRange.hpp"

namespace erbsland::re::impl {

using namespace unit;

auto U32StringMatch::getContentForGroup(const CaptureGroup &group) const noexcept -> text::U32String {
    const auto beginIndex = CpIndex::fromSizeT(group.begin());
    const auto endIndex = CpIndex::fromSizeT(group.end());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(beginIndex <= endIndex, "Capture group begin index must not exceed end index"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        endIndex <= CpIndex::end(_text.length()), "Capture group end index out of bounds"_el);
    return _text.slice(CpRange{beginIndex, endIndex});
}

}
