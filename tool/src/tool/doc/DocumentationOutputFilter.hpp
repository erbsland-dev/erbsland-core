// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DoxygenWarningSuppression.hpp"

#include <erbsland/ItemIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>

#include <optional>

namespace tool::doc {

/// Filter and normalize Sphinx/Doxygen diagnostics without hiding unconfigured warnings.
/// @tested{DocumentationOutputFilterTest}
class DocumentationOutputFilter final {
public:
    /// Prepare a filter for one project.
    /// @param root Physical project root.
    /// @param showSuppressed Whether to display accepted diagnostics.
    explicit DocumentationOutputFilter(el::Path root, bool showSuppressed = false);

public: // filtering
    /// Add a validated suppression rule in matching order.
    /// @param sourceGlob Source path pattern.
    /// @param contains Required message substring.
    /// @param regex Optional additional regular expression.
    /// @param reason Explanation for suppressing this diagnostic.
    void addRule(const el::String &sourceGlob, el::String contains, const el::String &regex, el::String reason);
    /// Process one line without its newline.
    /// @param line Original tool output.
    /// @return A shortened diagnostic or no value when hidden.
    [[nodiscard]] auto filterLine(const el::String &line) -> std::optional<el::String>;
    /// Normalize project and generated paths.
    /// @param line Original line.
    /// @return The normalized text.
    [[nodiscard]] auto shortenPaths(const el::String &line) const -> el::String;
    /// Get summaries in first-suppressed order.
    /// @return Nonempty suppression reports.
    [[nodiscard]] auto summary() const -> el::StringList;

private:
    el::String _generatedPrefix;                ///< Cached generated-input path prefix.
    el::String _documentationPrefix;            ///< Cached documentation path prefix.
    el::String _extendedGeneratedPrefix;        ///< Cached Windows extended generated-input prefix, if applicable.
    el::String _extendedDocumentationPrefix;    ///< Cached Windows extended documentation prefix, if applicable.
    bool _showSuppressed;                       ///< Whether accepted warnings remain visible.
    el::List<DoxygenWarningSuppression> _rules; ///< Explicit ordered rules.
    el::List<el::ItemIndex> _suppressed;        ///< First-observed order.
};

}
