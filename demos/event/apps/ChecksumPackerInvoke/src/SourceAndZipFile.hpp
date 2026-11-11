// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/List.hpp>
#include <erbsland/LogPath.hpp>
#include <erbsland/Path.hpp>

namespace demo {

/// The source and zip file path and a matching log path.
struct SourceAndZipFile {
    el::Path source;     ///< The full path to the source directory.
    el::Path zipPath;    ///< The full path for the target zip file.
    el::LogPath logPath; ///< A log path with a matching but unique name.
};

/// A list of source and target pairs to pack.
using SourceAndZipFileList = el::List<SourceAndZipFile>;

}
