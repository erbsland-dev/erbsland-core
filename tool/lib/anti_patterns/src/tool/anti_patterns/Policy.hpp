// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ExceptionFromRule.hpp"
#include "RuleRegistry.hpp"

#include <erbsland/conf/Value.hpp>
#include <erbsland/List.hpp>
#include <erbsland/PathGlobPattern.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringLiteral.hpp>
#include <erbsland/StringMap.hpp>
#include <tool/frame/ProjectFiles.hpp>

namespace tool::anti_patterns {

/// Validated source roots, suffixes, and central exceptions.
class Policy final {
    /// Configuration key for file suffixes.
    static constexpr auto cFileSuffixes = el::StringLiteral{"file_suffixes"};
    /// Configuration key for reason.
    static constexpr auto cReason = el::StringLiteral{"reason"};
    /// Configuration key for rules.
    static constexpr auto cRules = el::StringLiteral{"rules"};

public:
    /// Parse the tool's validated configuration and compile directory patterns.
    void read(const el::conf::ValuePtr &configuration, const frame::ProjectFiles &files, const RuleRegistry &rules);

public: // accessors
    /// Resolve directory patterns against the current project tree, skipping links.
    /// @param files The confined project file service.
    /// @return Unique absolute scan roots in deterministic order.
    /// @throws el::conf::ConfError If a pattern matches no directories.
    [[nodiscard]] auto roots(const frame::ProjectFiles &files) const -> el::PathList;
    /// Get supported source suffixes.
    [[nodiscard]] auto suffixes() const noexcept -> const el::StringList & { return _suffixes; }
    /// Get validated grouped exceptions.
    [[nodiscard]] auto exceptions() const noexcept -> const el::List<ExceptionFromRule> & { return _exceptions; }
    /// Test whether a source path has a supported suffix.
    [[nodiscard]] auto accepts(const el::Path &path) const -> bool;
    /// Find the configured exception reason for one path and rule.
    [[nodiscard]] auto reason(const el::Path &path, const el::String &rule) const -> el::String;

private:
    /// Validate filename suffixes and compile confined source-directory patterns.
    void readSources(const el::conf::ValuePtr &configuration);
    /// Validate exception paths and rule identities, rejecting duplicate coverage.
    void readExceptions(const el::conf::ValuePtr &configuration, const RuleRegistry &rules);

private:
    el::conf::ValuePtr _sourceDirectories; ///< Source setting retained for directory discovery diagnostics.
    el::StringMap<el::PathGlobPatternPtr> _directoryPatterns; ///< Project-relative directory globs.
    el::StringList _suffixes;                                 ///< Supported dotted C++ suffixes.
    el::List<ExceptionFromRule> _exceptions;                  ///< Configured accepted locations.
};

}
