// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Option_fwd.hpp"
#include "../OptionErrorContext.hpp"
#include "../Options_fwd.hpp"

#include "../../i18n/DisplayTextMap_fwd.hpp"
#include "../../text/StringView.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextNode_fwd.hpp"
#include "../../text/TextNodeType.hpp"
#include "../../unit/ArgumentUnit.hpp"
#include "../../unit/ColumnCount.hpp"
#include "../../unit/ElementIndex.hpp"
#include "../../unit/LineCount.hpp"
#include "../../unit/LineIndex.hpp"

#include <vector>

namespace erbsland::options::impl {

struct OptionDisplayGroup;
class OptionDisplayModel;
struct OptionDisplayRow;

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
    [[nodiscard]] auto helpDocument(text::StringView moduleName) const -> text::TextDocument;
    /// Build the application version document.
    [[nodiscard]] auto versionDocument(text::StringView moduleName) const -> text::TextDocument;
    /// Build an option error document.
    [[nodiscard]] auto errorDocument(const OptionErrorContext &errorContext) const -> text::TextDocument;

private:
    static constexpr auto cMaximumArgumentLines = unit::LineCount{12U};
    static constexpr auto cPreferredLinesBeforeError = unit::LineCount{5U};

    [[nodiscard]] static auto lineIndexFromArgumentIndex(unit::ArgumentIndex index) noexcept -> unit::LineIndex;
    [[nodiscard]] static auto elementIndexFromLineIndex(unit::LineIndex index) noexcept -> unit::ElementIndex;
    [[nodiscard]] static auto markerLength(text::StringView text) noexcept -> unit::ColumnCount;
    [[nodiscard]] auto defaultErrorTitle(OptionErrorReason reason) const -> text::StringView;

    void appendHeading(const text::TextNodePtr &parent, text::StringView title) const;
    void appendUsage(const text::TextNodePtr &parent, const OptionDisplayModel &model) const;
    void appendPlaceholder(
        const text::TextNodePtr &parent,
        text::TextNodeType type,
        text::StringView placeholder,
        text::StringView style) const;
    void appendOptionName(const text::TextNodePtr &termName, const OptionPtr &option) const;
    void appendOptionValuePlaceholder(const text::TextNodePtr &termName, const OptionPtr &option) const;
    void appendUsageOption(const text::TextNodePtr &termName, const OptionPtr &option) const;
    void appendOptionTermName(const text::TextNodePtr &termName, const OptionPtr &option) const;
    void appendOptionDescription(const text::TextNodePtr &item, const OptionDisplayRow &row) const;
    void appendOptionGroups(const text::TextNodePtr &parent, const std::vector<OptionDisplayGroup> &groups) const;
    void appendRowsAsTermList(
        const text::TextNodePtr &parent,
        const std::vector<OptionDisplayRow> &rows,
        bool optionRows,
        bool nestedDetails = false) const;
    void appendErrorSource(text::TextDocument &document, const OptionErrorContext &context) const;
    void appendCommandLineSnippet(text::TextDocument &document, const OptionErrorContext &context) const;
    [[nodiscard]] auto appendContextHelp(text::TextDocument &document, const OptionErrorContext &context) const -> bool;
    void appendFullHelpCommand(text::TextDocument &document, const OptionErrorContext &context) const;

private:
    OptionsPtr _options;                       ///< The options root to use for document building.
    i18n::DisplayTextMapConstPtr _displayText; ///< The wording configuration.
};

}
