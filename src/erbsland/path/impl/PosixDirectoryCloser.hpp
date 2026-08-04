// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <dirent.h>

namespace erbsland::path::impl {

/// Closes a POSIX directory stream when its owner is destroyed.
/// @tested{PosixPathOperationsTest}
class PosixDirectoryCloser final {
public:
    /// Close the directory stream if it is valid.
    void operator()(DIR *directory) const noexcept {
        if (directory != nullptr) {
            // A noexcept deleter cannot report failure and the directory stream cannot be reused after this call.
            ::closedir(directory);
        }
    }
};

}
