// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "String.hpp"
#include "StringList.hpp"

#include "../unit/LineIndex.hpp"

namespace erbsland::text {

/// A line-oriented excerpt of source code.
/// The line index is zero-based; renderers present it in the customary one-based form.
/// @tested{TextDocumentTest StringSourceTest TextStreamSourceTest}
struct CodeSnippet final {
    StringList lines;                                   ///< The source lines without line endings.
    unit::LineIndex startLine{unit::LineIndex::zero()}; ///< The zero-based index of the first source line.
    String language;                                    ///< The optional language identifier.
};

}
