// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>

namespace tool::security_hashes::config {

using namespace el::text::literals;

/// Configuration key for output file.
inline constexpr auto cOutputFile = "output_file"_el;
/// Configuration key for directories.
inline constexpr auto cDirectories = "directories"_el;
/// Configuration key for files.
inline constexpr auto cFiles = "files"_el;
/// Configuration key for included file names.
inline constexpr auto cIncludedFileNames = "included_file_names"_el;
/// Configuration key for included file suffixes.
inline constexpr auto cIncludedFileSuffixes = "included_file_suffixes"_el;
/// Configuration key for excluded directory paths.
inline constexpr auto cExcludedDirectoryPaths = "excluded_directory_paths"_el;
/// Configuration key for excluded directory names.
inline constexpr auto cExcludedDirectoryNames = "excluded_directory_names"_el;
/// Configuration key for excluded file names.
inline constexpr auto cExcludedFileNames = "excluded_file_names"_el;
/// Configuration key for excluded file suffixes.
inline constexpr auto cExcludedFileSuffixes = "excluded_file_suffixes"_el;

}
