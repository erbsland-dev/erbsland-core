// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ActionReference.hpp"
#include "WorkflowLine.hpp"

#include <erbsland/List.hpp>
#include <erbsland/Map.hpp>
#include <erbsland/Path.hpp>

namespace tool::github_workflows {

/// Parse ordinary block-style uses scalars and stage byte-preserving workflow edits.
class WorkflowFile final {
public:
    /// Parse a workflow without changing its text.
    /// @param path Workflow display path.
    /// @param text Complete original UTF-8 contents.
    /// @throws el::ApplicationError If a uses value has unsupported syntax.
    WorkflowFile(el::Path path, const el::String &text);

public: // accessors
    /// Get the workflow path.
    [[nodiscard]] auto path() const -> const el::Path & { return _path; }
    /// Get external action references in physical source order.
    [[nodiscard]] auto references() const -> const el::List<ActionReference> & { return _references; }
    /// Test for the exact canonical version annotation immediately before a reference.
    /// @param reference Original action reference.
    /// @param version Expected stable version tag.
    [[nodiscard]] auto hasAnnotation(const ActionReference &reference, const el::String &version) const -> bool;

public: // edits
    /// Stage a reference and its adjacent version comment without shifting source indices.
    /// @param reference Original action reference.
    /// @param sha Replacement pin; empty preserves the original uses value.
    /// @param version Expected stable annotation tag.
    void repair(const ActionReference &reference, const el::String &sha, const el::String &version);
    /// Assemble all staged changes, preserving untouched bytes and line terminators.
    [[nodiscard]] auto text() const -> el::String;

public: // validation
    /// Test a complete external action path with at least owner and repository components.
    [[nodiscard]] static auto isAction(const el::String &text) -> bool;
    /// Test a full 40-digit hexadecimal SHA pin.
    [[nodiscard]] static auto isSha(const el::String &text) -> bool;

private:
    /// Split physical lines once and preserve their original newline bytes.
    void splitLines(const el::String &text);
    /// Recognize external references outside comments and block scalars.
    void parse();
    /// Recognize action mapping keys in flow collections while skipping quoted values and comments.
    [[nodiscard]] static auto containsFlowUses(const el::String &text) -> bool;
    /// Test whether the preceding line is a managed action version annotation.
    [[nodiscard]] auto annotationLine(const ActionReference &reference) const -> el::ItemIndex;
    /// Build one canonical annotation.
    [[nodiscard]] static auto annotation(const ActionReference &reference, const el::String &version) -> el::String;
    /// Report unsupported workflow input with physical location.
    [[noreturn]] void fail(el::ItemIndex line, const el::String &description) const;

private:
    el::Path _path;                                   ///< Workflow filename for diagnostics.
    el::List<WorkflowLine> _lines;                    ///< Original physical lines.
    el::List<ActionReference> _references;            ///< Immutable parsed uses values.
    el::Map<el::ItemIndex, el::String> _replacements; ///< Replacement physical-line contents.
    el::Map<el::ItemIndex, el::String> _insertions;   ///< Comments inserted immediately before a uses line.
};

}
