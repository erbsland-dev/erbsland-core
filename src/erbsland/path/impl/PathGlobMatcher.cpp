// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathGlobMatcher.hpp"

#include "../../text/impl/ComponentGlobMatcher.hpp"
#include "../../unit/ItemCount.hpp"

#include <algorithm>

namespace erbsland::path::impl {

PathGlobMatcher::PathGlobMatcher(const Path &path, const PathGlobMatchMode mode, const bool descendant) :
    _path{path}, _mode{mode}, _descendant{descendant} {
    if (path.isAbsolute()) {
        ++_firstComponent;
    }
}

auto PathGlobMatcher::matches(const PathGlobData::Alternative &alternative) -> bool {
    const auto suffix = _mode == PathGlobMatchMode::Suffix && !alternative.anchored;
    if (suffix && _descendant) {
        return true; // An unanchored suffix can begin below any directory.
    }
    if (!alternative.hasRecursiveWildcard) {
        return matchFixed(alternative, suffix);
    }
    return matchRecursive(alternative, suffix);
}

auto PathGlobMatcher::matchFixed(const PathGlobData::Alternative &alternative, const bool suffix) const -> bool {
    const auto patternLength = unit::ItemCount::fromSizeT(alternative.components.size());
    auto comparedLength = _path.elementCount() - _firstComponent.distanceFromZero();
    auto pathIndex = _firstComponent;
    if (suffix) {
        if (comparedLength < patternLength) {
            return false;
        }
        pathIndex = unit::ItemIndex::end(_path.elementCount() - patternLength);
        comparedLength = patternLength;
    } else if (_descendant) {
        if (comparedLength >= patternLength) {
            return false; // A strict descendant requires at least one remaining pattern component.
        }
    } else if (comparedLength != patternLength) {
        return false;
    }
    for (auto patternIndex = std::size_t{}; patternIndex < comparedLength.toSizeT(); ++patternIndex, ++pathIndex) {
        if (!alternative.components[patternIndex].matches(_path.element(pathIndex))) {
            return false;
        }
    }
    return true;
}

auto PathGlobMatcher::matchRecursive(const PathGlobData::Alternative &alternative, const bool suffix) -> bool {
    auto matcher = text::impl::ComponentGlobMatcher{};
    matcher.start(alternative.components);
    for (auto index = _firstComponent; index.isWithin(_path.elementCount()); ++index) {
        matcher.consume(alternative.components, _path.element(index), suffix);
    }
    return matcher.accepts(_descendant);
}

}
