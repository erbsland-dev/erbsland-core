// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PathGlobPattern.hpp"

#include "Path.hpp"

#include "impl/PathGlobCompiler.hpp"
#include "impl/PathGlobData.hpp"

#include <utility>

namespace erbsland::path {

using namespace text;

PathGlobPattern::PathGlobPattern(std::unique_ptr<const impl::PathGlobData> data) : _data{std::move(data)} {
}

PathGlobPattern::~PathGlobPattern() = default;

auto PathGlobPattern::compile(AnyString pattern, const PathGlobPatternFlags flags) -> PathGlobPatternPtr {
    return impl::PathGlobCompiler{pattern, flags}.compile();
}

auto PathGlobPattern::fullMatch(const Path &path) const -> bool {
    return match(path, PathGlobMatchMode::Full);
}

auto PathGlobPattern::suffixMatch(const Path &path) const -> bool {
    return match(path, PathGlobMatchMode::Suffix);
}

auto PathGlobPattern::match(const Path &path, const PathGlobMatchMode mode) const -> bool {
    return _data->match(path, mode, false);
}

auto PathGlobPattern::mayMatchDescendant(const Path &path, const PathGlobMatchMode mode) const -> bool {
    return _data->match(path, mode, true);
}

auto PathGlobPattern::fullMatch(const String &text) const -> bool {
    return fullMatch(Path{text});
}

auto PathGlobPattern::suffixMatch(const String &text) const -> bool {
    return suffixMatch(Path{text});
}

auto PathGlobPattern::match(const String &text, const PathGlobMatchMode mode) const -> bool {
    return match(Path{text}, mode);
}

auto PathGlobPattern::mayMatchDescendant(const String &text, const PathGlobMatchMode mode) const -> bool {
    return mayMatchDescendant(Path{text}, mode);
}

}
