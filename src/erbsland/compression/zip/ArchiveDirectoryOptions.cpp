// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveDirectoryOptions.hpp"

#include "../../path/PathGlobPattern.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::compression::zip {

using namespace text;
using namespace text::literals;

auto ArchiveDirectoryOptions::setIncludeGlobs(const String &value) -> ArchiveDirectoryOptions & {
    return setIncludeGlobs(compileGlobs(value));
}

auto ArchiveDirectoryOptions::setIncludeGlobs(const StringList &value) -> ArchiveDirectoryOptions & {
    if (value.isEmpty()) {
        return setIncludeGlobs(nullptr);
    }
    return setIncludeGlobs(value.join(";"_el));
}

auto ArchiveDirectoryOptions::setExcludeGlobs(const String &value) -> ArchiveDirectoryOptions & {
    return setExcludeGlobs(compileGlobs(value));
}

auto ArchiveDirectoryOptions::setExcludeGlobs(const StringList &value) -> ArchiveDirectoryOptions & {
    if (value.isEmpty()) {
        return setExcludeGlobs(nullptr);
    }
    return setExcludeGlobs(value.join(";"_el));
}

auto ArchiveDirectoryOptions::compileGlobs(const String &value) -> path::PathGlobPatternPtr {
    using Flag = path::PathGlobPatternFlag;
    return path::PathGlobPattern::compile(value, {Flag::Default, Flag::MultiplePatterns, Flag::SemicolonSeparator});
}

}
