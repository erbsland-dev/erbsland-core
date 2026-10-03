// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Document.hpp"

namespace erbsland::conf::syntax {

/// Parse complete source snapshots without evaluating configuration directives.
/// @tested{SyntaxParserTest}
class Parser final {
public:
    /// Parse text, retaining source errors as diagnostics.
    /// @param text Original UTF-8 source snapshot.
    /// @param identifier Optional source identity, defaulting to an in-memory source.
    /// @return A lossless document, including when the input contains syntax errors.
    [[nodiscard]] auto parseText(text::String text, SourceIdentifierPtr identifier = {}) const -> DocumentPtr;
};

}
