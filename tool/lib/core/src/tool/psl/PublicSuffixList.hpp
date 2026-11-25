// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Rule.hpp"

#include "../idna/LabelProcessor.hpp"

#include <erbsland/LineIndex.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Literals.hpp>

namespace tool::psl {

using namespace el::text::literals;

/// Parse both PSL sections into unique, canonical ASCII rules.
/// @tested{PublicSuffixTest PublicSuffixFullTest}
class PublicSuffixList final {
    /// Comment prefix locating the snapshot version in the Public Suffix List header.
    static constexpr auto cVersionLabel = "// VERSION: "_el;

    /// Track the ordered ICANN and PRIVATE sections while parsing.
    /// @tested{PublicSuffixFullTest}
    struct ParseState final {
        unsigned section{}; ///< Current section, or zero between sections.
        bool seenIcann{};   ///< Whether the ICANN section has started.
        bool seenPrivate{}; ///< Whether the PRIVATE section has started.
    };

public:
    /// Parse a verified UTF-8 snapshot, retaining source locations in diagnostics.
    /// @param text Verified snapshot contents.
    /// @param source Diagnostic source path.
    /// @param processor Source-backed Unicode preparation.
    /// @throws el::ApplicationError If snapshot structure or rules are invalid.
    PublicSuffixList(const el::String &text, const el::String &source, const idna::LabelProcessor &processor);
    /// Get the upstream snapshot version.
    [[nodiscard]] auto version() const -> const el::String & { return _version; }
    /// Get lexicographically ordered, reversed-label rules.
    [[nodiscard]] auto rules() const -> const el::List<Rule> & { return _rules; }

private:
    /// Parse and validate the unique snapshot version header.
    /// @param line The trimmed header line.
    /// @param lineIndex The source line index.
    void parseVersion(const el::String &line, el::LineIndex lineIndex);
    /// Apply section markers and reject an invalid section order.
    /// @param line The trimmed comment line.
    /// @param lineIndex The source line index.
    /// @param state The current section state.
    void parseSection(const el::String &line, el::LineIndex lineIndex, ParseState &state) const;
    /// Validate rule syntax and prepare its reversed canonical ASCII text.
    /// @param line The trimmed rule line.
    /// @param lineIndex The source line index.
    /// @param processor Source-backed Unicode preparation.
    /// @return The canonical rule with its terminal token.
    [[nodiscard]] auto parseRule(
        const el::String &line, el::LineIndex lineIndex, const idna::LabelProcessor &processor) const -> Rule;
    /// Reverse canonical domain labels for suffix-oriented ordering.
    /// @param text The canonical ASCII domain.
    /// @return The domain with its labels in reverse order.
    [[nodiscard]] static auto reverseLabels(const el::String &text) -> el::String;
    /// Report invalid snapshot structure or rule syntax at its source line.
    /// @param message The reason parsing failed.
    /// @param line The zero-based snapshot line.
    [[noreturn]] void throwError(const el::String &message, el::LineIndex line) const;

private:
    el::String _source;    ///< Diagnostic source path.
    el::String _version;   ///< Snapshot version.
    el::List<Rule> _rules; ///< All canonical rules.
};

}
