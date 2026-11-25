// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DemoExecutor.hpp"
#include "DemoFingerprint.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/RegEx.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

/// Maintain author-controlled demo metadata and generated source/output blocks.
class DemoSynchronizer final {
    /// Author metadata for one validated managed demo block.
    struct DemoBlock {
        el::StringMap<el::String> options; ///< Unique option names and their values.
        el::StringList order;              ///< Author ordering retained when rendering metadata.
    };

    /// Demo metadata key selecting the source file used for excerpts and fingerprints.
    static constexpr auto cSource = "source"_el;
    /// Demo metadata key listing fixture files used for execution and fingerprints.
    static constexpr auto cFiles = "files"_el;

public:
    /// One ordered regular-expression replacement applied to extracted demo source.
    struct TextReplacement {
        el::re::RegExPtr pattern; ///< Compiled pattern.
        el::String replacement;   ///< Core replacement expression.
    };
    /// Ordered source presentation rules.
    using TextReplacements = el::List<TextReplacement>;

    /// Bind demo processing to a project file service.
    /// @param files Confined project access.
    /// @param executableRoot Project-relative or absolute directory containing demo executables.
    /// @param replacements Source presentation rules, applied after source selection.
    explicit DemoSynchronizer(
        const frame::ProjectFiles &files,
        el::Path executableRoot = el::Path{"cmake-build-debug/demo-apps"_el},
        TextReplacements replacements = {});

public: // processing
    /// Process every managed demo block in a document.
    /// @param document Owning RST path.
    /// @param text Original document text.
    /// @param force Regenerate even when source fingerprints match.
    /// @param issues Collected contextual generation diagnostics.
    /// @return Updated document.
    auto process(const el::Path &document, const el::String &text, bool force, el::StringList &issues) -> el::String;
    /// Migrate legacy metadata without extracting source or running demos.
    /// @param text Original document text.
    /// @return Metadata-migrated text.
    static auto migrate(const el::String &text) -> el::String;

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Parse one managed block, leaving the index at its closing directive.
    static auto readBlock(const el::StringList &lines, el::ItemIndex &index) -> DemoBlock;
    /// Reject execution metadata on template sources, including cached blocks.
    void validateTemplateOptions(const el::StringMap<el::String> &options) const;
    /// Validate a source path and its input size before reading.
    auto sourcePath(const el::String &name) const -> el::Path;
    /// Read a source path confined below demos.
    auto source(const el::String &name) const -> el::String;
    /// Fingerprint canonical author metadata, source, and fixture contents.
    auto fingerprint(const el::Path &document, const el::StringMap<el::String> &options) const -> el::String;
    /// Add resolved fixture arguments and contents in command order.
    void addFixtureInputs(DemoFingerprint &fingerprint, const el::Path &document, const el::String &filesText) const;
    /// Add demo source arguments referenced by execution commands.
    void addExecutionInputs(DemoFingerprint &fingerprint, const el::StringMap<el::String> &options) const;
    /// Generate the complete body of one block.
    auto generate(
        const el::Path &document,
        const el::StringMap<el::String> &options,
        const el::StringList &order,
        const el::String &indent,
        el::StringList &issues) -> el::StringList;
    /// Append selected source after applying presentation replacements in order.
    void appendSource(el::StringList &body, const el::StringMap<el::String> &options, const el::String &indent) const;
    /// Select execution keys and sort their parsed numeric order once.
    static auto executionKeys(const el::StringMap<el::String> &options) -> el::StringList;
    /// Append bounded execution results while collecting per-run diagnostics.
    void appendExecutions(
        el::StringList &body,
        const el::StringMap<el::String> &options,
        const el::StringMap<el::Path> &fixtures,
        const el::String &indent,
        el::StringList &issues);
    /// Preserve author option order, excluding the generated fingerprint.
    static void appendOptions(
        el::StringList &output,
        const el::StringMap<el::String> &options,
        const el::StringList &order,
        const el::String &indent);
    /// Append a directive and its indented content.
    static void appendDirective(
        el::StringList &output, const el::String &indent, const el::String &directive, const el::StringList &content);

private:
    const frame::ProjectFiles &_files; ///< Confined project inputs and outputs.
    DemoExecutor _executor;            ///< Validated native demo execution.
    TextReplacements _replacements;    ///< Ordered source presentation rules.
};

}
