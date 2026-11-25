// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>
#include <erbsland/String.hpp>

namespace tool::cleanup::config {

using namespace el::text::literals;

/// Configuration key selecting the library source root used for public-include resolution.
inline constexpr auto cLibraryDirectory = "library_directory"_el;
/// Configuration key selecting the source-to-public-header mapping used to rewrite includes.
inline constexpr auto cPublicHeaderMap = "public_header_map"_el;
/// Configuration key selecting the include prefix for public header spellings.
inline constexpr auto cPublicIncludePrefix = "public_include_prefix"_el;
/// Whether another library publishes headers below the same include prefix.
inline constexpr auto cSharedIncludePrefix = "shared_include_prefix"_el;
/// Configuration key listing source-file suffixes eligible for cleanup.
inline constexpr auto cFileSuffixes = "file_suffixes"_el;
/// Configuration key listing directory and file names skipped during cleanup.
inline constexpr auto cExcludedNames = "excluded_names"_el;
/// Configuration key containing the named source-tree cleanup policies.
inline constexpr auto cSources = "sources"_el;
/// Validation rule for any named source policy.
inline constexpr auto cSourcePolicyRule = "sources.vr_any"_el;
/// Project-relative input directory globs for each named source policy.
inline constexpr auto cSourceDirectoriesRule = "sources.vr_any.directories"_el;
/// Include style for each named source policy.
inline constexpr auto cSourceIncludeStyleRule = "sources.vr_any.include_style"_el;
/// Additional local include roots for each named source policy.
inline constexpr auto cSourceIncludeRootsRule = "sources.vr_any.include_roots"_el;
/// Configuration key containing CMake maintenance settings and target policies.
inline constexpr auto cCmake = "cmake"_el;
/// Configuration key setting the minimum CMake version emitted by cleanup.
inline constexpr auto cCmakeMinimumVersion = "cmake.minimum_version"_el;
/// Configuration key containing the named CMake source-tree policies.
inline constexpr auto cCmakeTrees = "cmake.trees"_el;
/// Validation rule for any named CMake policy.
inline constexpr auto cCmakePolicyRule = "cmake.trees.vr_any"_el;
/// Project-relative directory for each named CMake policy.
inline constexpr auto cCmakeDirectoryRule = "cmake.trees.vr_any.directory"_el;
/// Target name for each named CMake policy.
inline constexpr auto cCmakeTargetRule = "cmake.trees.vr_any.target"_el;
/// Configuration key containing clang-format settings and the incremental cache path.
inline constexpr auto cFormat = "format"_el;
/// Configuration key selecting the clang-format executable.
inline constexpr auto cFormatExecutable = "format.executable"_el;
/// Configuration key selecting the clang-format style file.
inline constexpr auto cFormatStyleFile = "format.style_file"_el;
/// Configuration key selecting the file-fingerprint cache for incremental formatting.
inline constexpr auto cFormatCacheFile = "format.cache_file"_el;
/// CMake policy key selecting the project-relative source-tree directory.
inline constexpr auto cDirectory = "directory"_el;
/// Source-policy key listing project-relative input directory globs.
inline constexpr auto cDirectories = "directories"_el;
/// Source-policy key selecting relative or public include spelling.
inline constexpr auto cIncludeStyle = "include_style"_el;
/// Source-policy key listing additional roots for local include resolution.
inline constexpr auto cIncludeRoots = "include_roots"_el;
/// CMake policy key naming the target that owns the source files.
inline constexpr auto cTarget = "target"_el;
/// Include-style value selecting paths relative to the including source file.
inline constexpr auto cRelativeIncludeStyle = "relative"_el;
/// Include-style value selecting canonical public header paths.
inline constexpr auto cPublicIncludeStyle = "public"_el;

/// Default library directory.
inline constexpr auto cDefaultLibraryDirectory = "src/erbsland"_el;
/// Default public header map.
inline constexpr auto cDefaultPublicHeaderMap = "tool/data/public_headers/header_map.txt"_el;
/// Default public include prefix.
inline constexpr auto cDefaultPublicIncludePrefix = "erbsland"_el;
/// Default CMake version.
inline constexpr auto cDefaultCmakeVersion = "3.28"_el;
/// Default formatter.
inline constexpr auto cDefaultFormatter = "clang-format"_el;
/// Default style file.
inline constexpr auto cDefaultStyleFile = ".clang-format"_el;
/// Default cache file.
inline constexpr auto cDefaultCacheFile = ".cache/code_cleanup/fingerprints.txt"_el;

}
