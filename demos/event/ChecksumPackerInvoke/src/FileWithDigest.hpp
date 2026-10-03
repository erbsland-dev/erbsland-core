// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/ByteBlock.hpp>
#include <erbsland/List.hpp>
#include <erbsland/Path.hpp>

namespace demo {

/// One entry with a found file and digest.
struct FileWithDigest {
    el::Path filePath;    ///< The path to the scanned file.
    el::ByteBlock digest; ///< The SHA3-256 digest of the file contents.
};

/// Results transferred from the scanner to the main event loop and then to the packer.
using FileWithDigestList = el::List<FileWithDigest>;

}
