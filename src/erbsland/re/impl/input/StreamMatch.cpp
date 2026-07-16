// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "StreamMatch.hpp"

#include "../error/InternalError.hpp"

namespace erbsland::re::impl {

auto StreamMatch::getContentForGroup(const CaptureGroup &group) const noexcept -> text::StringView {
    const auto index = static_cast<std::size_t>(group.index());
    ERBSLAND_CORE_RE_REQUIRE_SAFETY(index < _content.size(), "Capture group content index out of bounds"_el);
    return _content[index];
}

}
