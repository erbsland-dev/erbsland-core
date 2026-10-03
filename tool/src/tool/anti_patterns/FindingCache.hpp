// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Finding.hpp"
#include "RuleRegistry.hpp"

#include "../frame/ProjectFiles.hpp"

#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathInfo.hpp>
#include <erbsland/String.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>

#include <memory>

namespace tool::anti_patterns {

using namespace el::text::literals;

/// Disposable versioned cache of source-level findings without configured exceptions.
class FindingCache final {
    /// Cache field identifying the tool binary that produced the findings.
    constexpr static auto cBinaryField = "binary"_el;
    /// Cache field holding the rule-configuration digest for invalidation.
    constexpr static auto cConfigurationField = "configuration"_el;
    /// Finding field holding the exclusive byte offset of the matched range.
    constexpr static auto cEndField = "end"_el;
    /// Finding field holding the final matched line number.
    constexpr static auto cEndLineField = "end_line"_el;
    /// Cache field containing the per-file records.
    constexpr static auto cFilesField = "files"_el;
    /// File-record field containing findings before configured exceptions are applied.
    constexpr static auto cFindingsField = "findings"_el;
    /// Finding field retaining the source-level suppression reason.
    constexpr static auto cInlineReasonField = "inline_reason"_el;
    /// Finding field holding the first matched line number.
    constexpr static auto cLineField = "line"_el;
    /// File-record field identifying the scanned source path.
    constexpr static auto cPathField = "path"_el;
    /// Finding field identifying the detector that reported the match.
    constexpr static auto cRuleField = "rule"_el;
    /// File-record field holding the source metadata signature for invalidation.
    constexpr static auto cSignatureField = "signature"_el;
    /// Finding field retaining the matched source excerpt for reports.
    constexpr static auto cSnippetField = "snippet"_el;
    /// Finding field holding the inclusive byte offset of the matched range.
    constexpr static auto cStartField = "start"_el;
    /// Cache field identifying the supported serialization format.
    constexpr static auto cVersionField = "version"_el;

    /// One cached file signature and its source-level findings.
    struct Record final {
        el::String signature;
        el::List<Finding> findings;
    };
    using RecordPtr = std::shared_ptr<Record>; ///< Stable ownership of a cached file's findings.

public:
    /// Load a compatible cache, ignoring absent or malformed data.
    FindingCache(const frame::ProjectFiles &files, const RuleRegistry &rules);

public: // records
    /// Return valid cached findings, borrowed until the record is replaced or removed.
    [[nodiscard]] auto get(const el::Path &path, const el::String &signature) const -> const el::List<Finding> *;
    /// Replace one cached file record.
    void put(const el::Path &path, el::String signature, el::List<Finding> findings);
    /// Remove cache records outside the current full inventory.
    void retain(const el::StringSet &paths);
    /// Atomically write changed cache records.
    void save() const;
    /// Build a metadata signature for a source file.
    [[nodiscard]] static auto signature(const el::Path &path) -> el::String;
    /// Build a metadata signature for a source file.
    [[nodiscard]] static auto signature(const el::PathInfo &info) -> el::String;

private:
    /// Read a compatible cache and discard malformed data.
    void load();
    /// Identify the running tool binary for cache invalidation.
    [[nodiscard]] auto binarySignature() const -> el::String;

private:
    const frame::ProjectFiles &_files; ///< Confined cache access.
    const RuleRegistry &_rules;        ///< Rule metadata for decoding.
    el::StringMap<RecordPtr> _records; ///< Cached path entries.
    el::String _binarySignature;       ///< Running executable metadata.
    bool _dirty = false;               ///< Cache needs replacement.
};

}
