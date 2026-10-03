// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathGlobData.hpp"

#include "PathGlobMatcher.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/impl/StringPatternData.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::path::impl {

using namespace text::literals;

auto PathGlobData::Component::matches(const text::String &text) const -> bool {
    if (pattern != nullptr) {
        return pattern->matches(text);
    }
    return literal == text;
}

auto PathGlobData::match(const Path &path, const PathGlobMatchMode mode, const bool descendant) const -> bool {
    if (mode != PathGlobMatchMode::Full && mode != PathGlobMatchMode::Suffix) {
        throw err::ParameterFailure{"Invalid path glob matching mode"_el, "mode"_el};
    }
    if (path.isEmpty()) {
        return false;
    }
    auto matcher = PathGlobMatcher{path, mode, descendant};
    for (const auto &alternative : alternatives) {
        if (matcher.matches(alternative)) {
            return true;
        }
    }
    return false;
}

}
