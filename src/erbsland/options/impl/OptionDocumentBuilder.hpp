// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "OptionDisplayGroup_fwd.hpp"
#include "OptionDisplayModel_fwd.hpp"
#include "OptionDisplayRow_fwd.hpp"

#include "../Option_fwd.hpp"
#include "../OptionErrorContext.hpp"
#include "../Options_fwd.hpp"

#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../text/String.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode_fwd.hpp"
#include "../../text/TextNodeType.hpp"
#include "../../unit/ArgumentUnit.hpp"
#include "../../unit/ColumnCount.hpp"
#include "../../unit/ItemIndex.hpp"
#include "../../unit/LineCount.hpp"
#include "../../unit/LineIndex.hpp"

#include <vector>

namespace erbsland::options::impl {

/// Builds neutral text documents for option help, version and error output.
/// @tested{OptionDocumentTest}
class OptionDocumentBuilder final {
public:
    /// Create a builder for the given options and wording.
    OptionDocumentBuilder(OptionsPtr options, i18n::DisplayTextMapConstPtr displayText);

    // defaults
    ~OptionDocumentBuilder() = default;
    OptionDocumentBuilder(const OptionDocumentBuilder &) = default;
    auto operator=(const OptionDocumentBuilder &) -> OptionDocumentBuilder & = default;
    OptionDocumentBuilder(OptionDocumentBuilder &&) = default;
    auto operator=(OptionDocumentBuilder &&) -> OptionDocumentBuilder & = default;

public:
    /// Build the help document for the selected module.
    [[nodiscard]] auto helpDocument(const text::String &moduleName) const -> text::TextDocument;
    /// Build the application version document.
    [[nodiscard]] auto versionDocument(const text::String &moduleName) const -> text::TextDocument;
    /// Build an option error document.
    [[nodiscard]] auto errorDocument(const OptionErrorContext &errorContext) const -> text::TextDocument;

private:
    static constexpr auto cMaximumArgumentLines = unit::LineCount{12U};
    static constexpr auto cPreferredLinesBeforeError = unit::LineCount{5U};

    /// Convert an argument index to its source line index.
    [[nodiscard]] static auto lineIndexFromArgumentIndex(unit::ArgumentIndex index) noexcept -> unit::LineIndex;
    /// Convert a source line index to its document element index.
    [[nodiscard]] static auto elementIndexFromLineIndex(unit::LineIndex index) noexcept -> unit::ItemIndex;
    /// Get the display width of an error-marker line.
    [[nodiscard]] static auto markerLength(const text::String &text) noexcept -> unit::ColumnCount;
    /// Get the localized default title for an error reason.
    [[nodiscard]] auto defaultErrorTitle(OptionErrorReason reason) const -> text::String;

    /// Append a document heading.
    void appendHeading(const text::TextNodePtr &parent, text::String title) const;
    /// Append a usage paragraph from the display model.
    void appendUsage(const text::TextNodePtr &parent, const OptionDisplayModel &model) const;
    /// Append styled placeholder text to a document node.
    void appendPlaceholder(
        const text::TextNodePtr &parent, text::TextNodeType type, text::String placeholder, text::String style) const;
    /// Append all visible names for an option.
    void appendOptionName(const text::TextNodePtr &termName, const OptionPtr &option) const;
    /// Append the displayed value placeholder for an option.
    void appendOptionValuePlaceholder(const text::TextNodePtr &termName, const OptionPtr &option) const;
    /// Append one option to the usage term.
    void appendUsageOption(const text::TextNodePtr &termName, const OptionPtr &option) const;
    /// Append a formatted option name to a term.
    void appendOptionTermName(const text::TextNodePtr &termName, const OptionPtr &option) const;
    /// Append the description and details for one option row.
    void appendOptionDescription(const text::TextNodePtr &item, const OptionDisplayRow &row) const;
    /// Append all option groups as document sections.
    void appendOptionGroups(const text::TextNodePtr &parent, const std::vector<OptionDisplayGroup> &groups) const;
    /// Append display rows as a term list.
    void appendRowsAsTermList(
        const text::TextNodePtr &parent,
        const std::vector<OptionDisplayRow> &rows,
        bool optionRows,
        bool nestedDetails = false) const;
    /// Append the source location associated with an option error.
    void appendErrorSource(text::TextDocument &document, const OptionErrorContext &context) const;
    /// Append a command-line excerpt that highlights an option error.
    void appendCommandLineSnippet(text::TextDocument &document, const OptionErrorContext &context) const;
    /// Append contextual help and report whether it was available.
    [[nodiscard]] auto appendContextHelp(text::TextDocument &document, const OptionErrorContext &context) const -> bool;
    /// Append the command required to display complete help.
    void appendFullHelpCommand(text::TextDocument &document, const OptionErrorContext &context) const;

private:
    OptionsPtr _options;                       ///< The options root to use for document building.
    i18n::DisplayTextMapConstPtr _displayText; ///< The wording configuration.
};

}
