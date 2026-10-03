// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Token.hpp"

#include "../../../util/List.hpp"

namespace erbsland::conf::syntax::impl {

/// One physical line of a valid statement, excluding its newline token.
/// @tested{SyntaxFormatterTest}
struct LayoutLine {
    util::List<TokenPtr> tokens; ///< Non-newline source tokens.
    text::String newline;        ///< Exact original line terminator.
};

}
