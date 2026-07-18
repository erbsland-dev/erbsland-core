// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StringMatch.hpp"

#include "../error/InternalError.hpp"

#include "../../../unit/ByteRange.hpp"

namespace erbsland::re::impl {

auto StringMatch::getContentForGroup(const CaptureGroup &group) const noexcept -> text::String {
    const auto beginIndex = unit::ByteIndex::fromSizeT(group.begin());
    const auto endIndex = unit::ByteIndex::fromSizeT(group.end());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(beginIndex <= endIndex, "Capture group begin index must not exceed end index"_el);
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(
        endIndex <= unit::ByteIndex::end(_text.length()), "Capture group end index out of bounds"_el);
    return _text.slice(unit::ByteRange{beginIndex, endIndex});
}

}
