// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../core/Definitions.hpp"

#include <cstdint>

namespace erbsland::path {

/// The mode to use when creating a file or directory.
enum class PathCreateMode : std::uint8_t {
    /// Create or overwrite.
    /// - For file streams: Create a new file or overwrite an existing file.
    /// - For directories: Create a new directory or keep an existing one.
    /// - For empty files: Create an empty file, overwriting an existing file with an empty one.
    CreateOrOverwrite,
    /// Create or append.
    /// - For file streams: Create a new file or append to an existing file.
    /// - For directories: Create a new directory or keep an existing one.
    /// - For empty files: Create an empty file, or update modification time of an existing file,
    ///   without changing the file content.
    CreateOrAppend,
    /// Only create new.
    /// - For file streams: Only create a new file. Fail if the file already exists.
    /// - For directories: Only create a new directory. Fail if the directory already exists.
    /// - For empty files: Only create a new empty file. Fail if the file already exists.
    CreateNew,
};

}
