// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/StringList.hpp>

namespace tool::anti_patterns {

/// One reasoned, path-scoped group of accepted rule findings.
struct ExceptionFromRule final {
    el::String path;                ///< Original project-relative glob.
    el::PathGlobPatternPtr pattern; ///< Compiled immutable path matcher.
    el::StringList rules;           ///< Accepted rule identifiers.
    el::String reason;              ///< Required explanation.
};

}
