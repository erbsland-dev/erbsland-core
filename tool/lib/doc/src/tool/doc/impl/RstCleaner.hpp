// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DoxygenIndex.hpp"

#include <erbsland/ItemIndex.hpp>

namespace tool::doc::impl {

/// Normalize RST headings, public API references, and simple paragraphs.
class RstCleaner final {
    /// State of code and generated demo blocks that bypass normal cleanup.
    struct Protection {
        bool code{};            ///< Inside an indentation-protected literal or source block.
        el::ByteCount indent{}; ///< Indentation of the opening directive.
        bool demo{};            ///< Inside a managed demo block.
    };

public:
    /// Create a cleaner using an existing symbol index.
    /// @param index Public API symbol index.
    /// @param lineWidth Maximum paragraph line width.
    RstCleaner(const DoxygenIndex &index, int lineWidth);

public: // processing
    /// Clean a document without changing code or generated demo content.
    /// @param text Original RST text.
    /// @param warnings Unresolved symbol diagnostics.
    /// @return Normalized document text.
    auto process(const el::String &text, el::StringList &warnings) const -> el::String;

private:
    /// Advance block protection and decide whether the line must be preserved verbatim.
    static auto preserveLine(Protection &state, const el::String &current) -> bool;
    /// Test a supported heading adornment.
    static auto adornment(const el::String &line) -> bool;
    /// Test whether a line belongs to a simple top-level paragraph.
    static auto paragraph(const el::StringList &lines, el::ItemIndex index) -> bool;
    /// Append a recognized heading with normalized adornments and advance past it.
    static auto appendHeading(const el::StringList &lines, el::ItemIndex &index, el::StringList &result) -> bool;
    /// Reflow one paragraph while preserving inline markup.
    auto wrap(const el::String &text) const -> el::StringList;

private:
    const DoxygenIndex &_index; ///< Loaded public API symbols.
    int _lineWidth;             ///< Maximum simple paragraph width.
};

}
