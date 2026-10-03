// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/StringPatternFactory.hpp"

namespace erbsland::text {

template <pattern::AnyElement... Args>
    requires((!std::is_same_v<std::remove_cvref_t<Args>, pattern::Star>) && ...)
auto StringPattern::fromDivider(const Args &...elements) -> StringPattern {
    auto result = StringPattern{};
    result._data = impl::createStaticStringPatternData(StringPatternKind::Divider, elements...);
    return result;
}

template <pattern::AnyElement... Args>
    requires((!std::is_same_v<std::remove_cvref_t<Args>, pattern::Divider>) && ...)
auto StringPattern::fromGlob(const Args &...elements) -> StringPattern {
    auto result = StringPattern{};
    result._data = impl::createStaticStringPatternData(StringPatternKind::Glob, elements...);
    return result;
}

}
