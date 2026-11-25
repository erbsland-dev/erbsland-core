// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ReferenceGroup.hpp"

#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/StringLiteral.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringSet.hpp>
#include <tool/frame/ProjectFiles.hpp>
#include <tool/frame/TaskProgress.hpp>

namespace tool::doc::impl {

using namespace el::text::literals;

/// Plan and apply managed reference interfaces, orphan removals, and index entries.
class ReferenceDocumentation final {
    /// Configuration key listing headers excluded from reference coverage globally or within a group.
    static constexpr auto cExcludedHeaderGlobs = "excluded_header_globs"_el;

    /// Configuration key for page.
    static constexpr auto cPage = el::StringLiteral{"page"};
    /// Configuration key for headers.
    static constexpr auto cHeaders = el::StringLiteral{"headers"};
    /// Configuration key for header globs.
    static constexpr auto cHeaderGlobs = el::StringLiteral{"header_globs"};
    /// Configuration key for manual reference pages.
    static constexpr auto cManualReferencePages = el::StringLiteral{"manual_reference_pages"};

public:
    /// Load validated reference configuration and expand its header selections.
    /// @param files Confined file access.
    /// @param configuration Validated command section.
    ReferenceDocumentation(const frame::ProjectFiles &files, const el::conf::ValuePtr &configuration);

public: // accessors
    /// Get public headers not assigned to any configured group.
    [[nodiscard]] auto unassignedHeaders() const noexcept -> const el::StringList & { return _unassigned; }

public: // generation
    /// Generate all managed pages and indexes, preserving hand-maintained prose.
    /// @param progress Optional shared task progress.
    void run(frame::TaskProgress progress = {});

private:
    /// Report a failure in this operation, optionally identifying its input file.
    [[noreturn]] static void throwError(const el::String &description, const el::String &path = {});

private:
    /// Resolve a configured page below the reference root, retaining its configuration location.
    auto referencePath(const el::conf::ValuePtr &value) const -> el::Path;
    /// Scan only selected headers and their forward declarations.
    auto scanHeaders() const -> el::StringMap<el::StringList>;
    /// Group directives and count remaining occurrences before choosing their final owner.
    auto collectInterfaces(const el::StringMap<el::StringList> &scannedHeaders, el::StringMap<int> &remaining) const
        -> el::StringMap<el::List<el::StringList>>;
    /// Render one interface, preserving prose and deduplicating shared directives.
    auto renderInterface(
        const ReferenceGroup &group, const el::List<el::StringList> &headers, el::StringMap<int> &remaining) const
        -> el::String;
    /// Assemble the complete parent-to-child page index.
    auto collectIndexes(const el::StringSet &expected) const -> el::StringMap<el::StringList>;
    /// Render one index while retaining hand-maintained text and options.
    auto renderIndex(const el::String &directory, const el::StringList &entries) const -> el::String;
    /// Publish a completely validated plan, then remove orphan pages.
    void publish(const el::StringMap<el::String> &outputs, const el::StringSet &expected) const;
    /// Test configured source/header exclusions.
    auto excluded(const el::Path &path) const -> bool;
    /// Expand configured wildcard selections.
    static auto matches(const el::conf::ValuePtr &patterns, const el::String &name) -> bool;
    /// Create a readable title from an identifier or path stem.
    static auto title(const el::String &name) -> el::String;
    /// Replace the final, uniquely identified Interface body.
    static auto replaceInterface(const el::String &text, const el::String &body) -> el::String;
    /// Replace one toctree's entries while preserving its options and surrounding prose.
    static auto replaceIndex(const el::String &text, el::StringList entries) -> el::String;

private:
    frame::TaskProgress _progress;     ///< Shared progress for header and page processing.
    const frame::ProjectFiles &_files; ///< Confined project inputs and outputs.
    el::conf::ValuePtr _configuration; ///< Validated exclusion policies.
    el::Path _source;                  ///< Public source root.
    el::Path _reference;               ///< Reference page root.
    el::List<ReferenceGroup> _groups;  ///< Validated reference groups.
    el::StringList _unassigned;        ///< Eligible headers lacking reference groups.
    el::StringList _manualPages;       ///< Hand-maintained pages retained during orphan cleanup.
};

}
