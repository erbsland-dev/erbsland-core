// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/path/Path.hpp>
#include <erbsland/text/StringList.hpp>

namespace erbsland::package {

/// One configured extra file or directory.
/// @notest{Covered by package integration tests.}
struct FileEntry final {
    path::Path path;               ///< Source relative to the configuration file.
    path::Path target;             ///< Destination relative to the package root.
    bool recursive{};              ///< Recurse into directories.
    text::StringList includes;     ///< Include glob patterns.
    text::StringList excludes;     ///< Exclude glob patterns.
    text::StringList includeRegex; ///< Include regular expressions.
    text::StringList excludeRegex; ///< Exclude regular expressions.
};

}
