// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Document.hpp"
#include "Format.hpp"
#include "FormatResult.hpp"

namespace erbsland::conf::syntax {

/// Format a syntax snapshot without evaluating or altering its semantic values by default.
/// @tested{SyntaxFormatterTest}
class Formatter final {
public:
    /// Use repository formatting defaults.
    Formatter() = default;
    /// Use a custom format.
    /// @param format Base style and selector rules.
    explicit Formatter(Format format) : _format{std::move(format)} {}

public: // formatting
    /// Format valid statements while retaining damaged statements verbatim.
    /// @param document Original source snapshot.
    /// @return Formatted text with original-source diagnostics.
    /// @throws err::ParameterFailure If the document is null or a selected style has invalid counts or separators.
    [[nodiscard]] auto format(const DocumentPtr &document) const -> FormatResult;

private:
    Format _format; ///< Immutable format configuration.
};

}
