// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/RegEx.hpp>

namespace tool::doc {

/// One source-specific accepted Doxygen diagnostic and its accumulated count.
/// @tested{DocumentationOutputFilterTest}
struct DoxygenWarningSuppression final {
    el::PathGlobPatternPtr source; ///< Case-sensitive source glob.
    el::String messageContains;    ///< Required message substring.
    el::re::RegExPtr messageRegex; ///< Optional additional message condition.
    el::String reason;             ///< Explanation shown in the summary.
    el::ItemCount count{};         ///< Suppressed diagnostics.
};

}
