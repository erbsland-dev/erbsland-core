// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>

namespace tool::headers::config {

using namespace el::text::literals;

/// Configuration key selecting the project-relative source-header root.
inline constexpr auto cSourceBaseDirectory = "source_base_directory"_el;
/// Configuration key selecting the project-relative generated-wrapper root.
inline constexpr auto cIncludeBaseDirectory = "include_base_directory"_el;
/// Configuration key selecting the canonical source-to-public-header mapping output.
inline constexpr auto cMappingFile = "mapping_file"_el;
/// Configuration key naming the root namespace for generated exports.
inline constexpr auto cBaseNamespace = "base_namespace"_el;
/// Configuration key enabling namespace declaration and export headers.
inline constexpr auto cGenerateNamespaceHeaders = "generate_namespace_headers"_el;
/// Configuration key selecting the generated namespace-declaration header.
inline constexpr auto cNamespacesHeader = "namespaces_header"_el;
/// Configuration key selecting the generated consumer namespace-export header.
inline constexpr auto cMakeOneNamespaceHeader = "make_one_namespace_header"_el;
/// Configuration key listing directory basenames excluded from publication and coverage checks.
inline constexpr auto cExcludedDirectories = "excluded_directories"_el;
/// Configuration key listing filename globs excluded from public wrappers.
inline constexpr auto cExcludedHeaders = "excluded_headers"_el;
/// Configuration key listing wrapper names allowed to aggregate multiple source headers.
inline constexpr auto cMergeableHeaderNames = "mergeable_header_names"_el;
/// Configuration key listing additional namespace declarations relative to the base namespace.
inline constexpr auto cDeclaredNamespaces = "declared_namespaces"_el;
/// Configuration key defining namespace aliases exported into the base namespace.
inline constexpr auto cNamespaceAliases = "namespace_aliases"_el;
/// Configuration key defining additional compatibility wrappers for source headers.
inline constexpr auto cHeaderAliases = "header_aliases"_el;
/// Configuration key listing line-comment prefixes that identify generated headers.
inline constexpr auto cGeneratedMarkers = "generated_markers"_el;
/// Configuration key containing inherited publication policies for source directories.
inline constexpr auto cDirectory = "directory"_el;
/// Directory-policy key indicating whether a child introduces a nested namespace.
inline constexpr auto cNestedNamespace = "nested_namespace"_el;
/// Directory-policy key enabling symbol exports into the flattened consumer namespace.
inline constexpr auto cFlatten = "flatten"_el;
/// Directory-policy key overriding the generated-wrapper destination directory.
inline constexpr auto cTargetDirectory = "target_directory"_el;
/// Directory-policy key overriding the namespace used for exported symbols.
inline constexpr auto cTargetNamespace = "target_namespace"_el;
/// Directory-policy key listing filename globs selected for publication.
inline constexpr auto cIncludedHeaders = "included_headers"_el;
/// Directory-policy key listing headers omitted from flattened namespace exports.
inline constexpr auto cExcludedFromFlattening = "excluded_from_flattening"_el;
/// Configuration key listing filename globs omitted from generated umbrella headers.
inline constexpr auto cExcludedFromAllHeaders = "excluded_from_all_headers"_el;
/// Directory-policy key listing declarations selected for namespace exports.
inline constexpr auto cIncludedSymbols = "included_symbols"_el;

}
