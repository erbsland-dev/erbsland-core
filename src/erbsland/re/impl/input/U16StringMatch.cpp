// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "U16StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../../unit/U16Range.hpp"

namespace erbsland::re::impl {

using namespace unit;

auto U16StringMatch::getContentForGroup(const CaptureGroup &group) const noexcept -> text::U16String {
    const auto beginIndex = U16Index::fromSizeT(group.begin());
    const auto endIndex = U16Index::fromSizeT(group.end());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(beginIndex <= endIndex, "Capture group begin index must not exceed end index"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        endIndex <= U16Index::end(_text.length()), "Capture group end index out of bounds"_el);
    return _text.slice(U16Range{beginIndex, endIndex});
}

}
