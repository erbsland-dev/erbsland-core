// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BlockStyle.hpp"
#include "../ParagraphIndents.hpp"
#include "../TerminalDocumentStyleRule.hpp"
#include "../TerminalDocumentStyleSelector.hpp"

#include "../../mem/SharedData.hpp"
#include "../../mem/SharedDataPointer.hpp"
#include "../../util/List.hpp"

namespace erbsland::cterm::impl {

/// Shared backing data for `TerminalDocumentStyle`.
class TerminalDocumentStyleData final : public mem::SharedData {
public:
    /// One explicit style definition.
    struct Entry final {
        TerminalDocumentStyleSelector selector; ///< The selector to match.
        TerminalDocumentStyleRule rule;         ///< The rule for this selector.
    };

    /// Style definition list with mutable internal access for stylesheet editing.
    class EntryList final : public util::List<Entry, EntryList> {
        using Base = util::List<Entry, EntryList>;

    public:
        using Base::Base;
        /// Access a mutable entry by index.
        /// @param index The entry index.
        /// @return The mutable entry.
        [[nodiscard]] auto mutableAt(Index index) -> Entry & { return mutableRaw()[index.toSizeT()]; }
    };

public:
    /// Create style data with the base defaults.
    TerminalDocumentStyleData();

    // defaults
    ~TerminalDocumentStyleData() = default;
    TerminalDocumentStyleData(const TerminalDocumentStyleData &) = default;
    TerminalDocumentStyleData(TerminalDocumentStyleData &&) = default;
    auto operator=(const TerminalDocumentStyleData &) -> TerminalDocumentStyleData & = default;
    auto operator=(TerminalDocumentStyleData &&) -> TerminalDocumentStyleData & = default;

public:
    BlockStyle baseTextStyle;         ///< Base text style.
    ParagraphIndents baseBlockLayout; ///< Base block layout.
    EntryList definitions;            ///< Explicit style definitions.
};

using TerminalDocumentStyleDataPtr = mem::SharedDataPointer<TerminalDocumentStyleData>;

}
