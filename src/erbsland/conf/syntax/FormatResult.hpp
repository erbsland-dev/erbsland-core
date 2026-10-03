// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Diagnostic.hpp"

#include "../../util/List.hpp"

namespace erbsland::conf::syntax {

/// Formatted source and diagnostics referring to the original input.
/// @tested{SyntaxFormatterTest}
class FormatResult final {
public:
    /// Construct a formatting result.
    /// @param text Formatted text.
    /// @param diagnostics Source errors and formatting warnings.
    FormatResult(text::String text, util::List<Diagnostic> diagnostics) :
        _text{std::move(text)}, _diagnostics{std::move(diagnostics)} {}

public: // accessors
    /// Get formatted source text.
    [[nodiscard]] auto text() const noexcept -> const text::String & { return _text; }
    /// Get diagnostics in original-source coordinates.
    [[nodiscard]] auto diagnostics() const noexcept -> const util::List<Diagnostic> & { return _diagnostics; }

private:
    text::String _text;                  ///< Formatted source.
    util::List<Diagnostic> _diagnostics; ///< Input errors and formatting warnings.
};

}
