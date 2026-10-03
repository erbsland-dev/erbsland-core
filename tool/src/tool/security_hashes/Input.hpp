// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Path.hpp>
#include <erbsland/String.hpp>

namespace tool::security_hashes {

/// One unique source file and the manifest records to which it contributes.
struct Input final {
    el::Path path;                ///< Absolute confined source path.
    el::String relative;          ///< Stable project-relative POSIX spelling.
    bool directoryRecord = false; ///< Include in its parent's direct-file hash.
    bool fileRecord = false;      ///< Emit an explicit file record.
};

}
