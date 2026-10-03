// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DirectoryPolicy.hpp"

#include <erbsland/conf/Value.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/StringList.hpp>
#include <erbsland/StringLiteral.hpp>
#include <erbsland/StringMap.hpp>
#include <erbsland/StringPattern.hpp>
#include <erbsland/StringSet.hpp>

namespace tool::headers {

/// Read and validate the complete directory-based public-header policy.
class HeaderPolicy final {
    /// Configuration key for source header.
    static constexpr auto cSourceHeader = el::StringLiteral{"source_header"};
    /// Configuration key for include header.
    static constexpr auto cIncludeHeader = el::StringLiteral{"include_header"};

public: // configuration
    /// Read configuration, resolving defaults and inheritance without accessing source files.
    /// @param configuration The command configuration section.
    void read(const el::conf::ValuePtr &configuration);
    /// Match filename-only Core glob patterns.
    /// @param name A filename.
    /// @param patterns Validated patterns.
    /// @return Whether any pattern matches.
    [[nodiscard]] auto matches(const el::String &name, const el::StringList &patterns) const -> bool;
    /// Require a portable ASCII relative path, with an optional empty root spelling.
    /// @param path Path to validate.
    /// @param allowEmpty Whether the empty path is allowed.
    /// @param value Originating setting, when validating configuration.
    static void validatePath(const el::String &path, bool allowEmpty = false, const el::conf::ValuePtr &value = {});
    /// Require a C++ identifier or qualified namespace name.
    /// @param name Name to validate.
    /// @param qualified Whether namespace separators are allowed.
    /// @param value Originating setting, when validating configuration.
    static void validateName(const el::String &name, bool qualified = false, const el::conf::ValuePtr &value = {});
    /// Stop with a public-header diagnostic.
    /// @param description Error details.
    /// @param value Originating setting, if this is a configuration failure.
    [[noreturn]] static void throwError(const el::String &description, const el::conf::ValuePtr &value = {});

public: // resolved policy
    /// Get the originating configuration for discovery diagnostics.
    [[nodiscard]] auto configuration() const -> const el::conf::ValuePtr & { return _configuration; }
    /// Get project-relative source base.
    [[nodiscard]] auto sourceBase() const -> const el::Path & { return _sourceBase; }
    /// Get project-relative wrapper base.
    [[nodiscard]] auto includeBase() const -> const el::Path & { return _includeBase; }
    /// Get project-relative mapping output.
    [[nodiscard]] auto mappingFile() const -> const el::Path & { return _mappingFile; }
    /// Get the library namespace.
    [[nodiscard]] auto baseNamespace() const -> const el::String & { return _baseNamespace; }
    /// Get source-relative namespace declaration output.
    [[nodiscard]] auto namespacesHeader() const -> const el::String & { return _namespacesHeader; }
    /// Get source-relative consumer export output.
    [[nodiscard]] auto flattenHeader() const -> const el::String & { return _flattenHeader; }
    /// Get directory basenames pruned during discovery.
    [[nodiscard]] auto excludedDirectories() const -> const el::StringList & { return _excludedDirectories; }
    /// Get global filename exclusions.
    [[nodiscard]] auto excludedHeaders() const -> const el::StringList & { return _excludedHeaders; }
    /// Get global filename exclusions from umbrellas only.
    [[nodiscard]] auto excludedFromAll() const -> const el::StringList & { return _excludedFromAll; }
    /// Get explicitly permitted aggregate wrapper names.
    [[nodiscard]] auto mergeableNames() const -> const el::StringSet & { return _mergeableNames; }
    /// Get additional fully qualified namespace declarations.
    [[nodiscard]] auto declaredNamespaces() const -> const el::StringSet & { return _declaredNamespaces; }
    /// Get base-namespace aliases to qualified namespaces.
    [[nodiscard]] auto namespaceAliases() const -> const el::StringMap<el::String> & { return _namespaceAliases; }
    /// Get explicit compatibility paths grouped by source header.
    [[nodiscard]] auto headerAliases() const -> const el::StringMap<el::StringSet> & { return _headerAliases; }
    /// Get effective policies indexed by source directory.
    [[nodiscard]] auto directories() const -> const el::StringMap<DirectoryPolicy> & { return _directories; }
    /// Get line-comment prefixes identifying generator-owned headers.
    [[nodiscard]] auto generatedMarkers() const -> const el::StringList & { return _generatedMarkers; }

private:
    /// Read and validate paths configuration.
    void readPaths(const el::conf::ValuePtr &configuration);
    /// Read and validate exclusions configuration.
    void readExclusions(const el::conf::ValuePtr &configuration);
    /// Read and validate aliases configuration.
    void readAliases(const el::conf::ValuePtr &configuration);
    /// Read and validate generatedmarkers configuration.
    void readGeneratedMarkers(const el::conf::ValuePtr &configuration);
    /// Test a validated relative path prefix at a slash boundary without allocating a prefix string.
    [[nodiscard]] static auto isChildPath(const el::Path &path, const el::Path &parent) -> bool;
    /// Resolve one directory section and its children, inheriting its parent's policy.
    void readDirectory(const el::conf::ValuePtr &section, const DirectoryPolicy *parent);
    /// Validate filename-only globs and compile each distinct pattern once.
    void compilePatterns(const el::StringList &patterns, const el::conf::ValuePtr &value);

private:
    el::conf::ValuePtr _configuration;           ///< Originating settings retained for discovery diagnostics.
    el::Path _sourceBase;                        ///< Project-relative source base.
    el::Path _includeBase;                       ///< Project-relative wrapper base.
    el::Path _mappingFile;                       ///< Project-relative mapping output.
    el::String _baseNamespace;                   ///< The library namespace.
    el::String _namespacesHeader;                ///< Source-relative namespace declaration output.
    el::String _flattenHeader;                   ///< Source-relative consumer export output.
    el::StringList _excludedDirectories;         ///< Directory basenames pruned during discovery.
    el::StringList _excludedHeaders;             ///< Global filename exclusions.
    el::StringList _excludedFromAll;             ///< Global exclusions from umbrellas only.
    el::StringSet _mergeableNames;               ///< Explicitly permitted aggregate wrapper names.
    el::StringSet _declaredNamespaces;           ///< Additional fully qualified namespace declarations.
    el::StringMap<el::String> _namespaceAliases; ///< Base-namespace aliases to qualified namespaces.
    el::StringMap<el::StringSet> _headerAliases; ///< Explicit compatibility paths grouped by source header.
    el::StringMap<DirectoryPolicy> _directories; ///< Effective policies indexed by source directory.

    el::StringList _generatedMarkers;            ///< Line-comment prefixes identifying generator-owned headers.
    el::StringMap<std::shared_ptr<const el::StringPattern>>
        _patterns;                               ///< Compiled globs shared by all effective policies.
};

}
