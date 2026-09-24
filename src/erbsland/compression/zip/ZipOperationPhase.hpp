// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::zip {

/// The archive operation phase in which a ZIP failure occurred.
enum class ZipOperationPhase : uint8_t {
    Open,              ///< Opening or buffering an archive.
    Directory,         ///< Locating or parsing the central directory.
    LocalHeader,       ///< Validating a local file header.
    Payload,           ///< Reading or copying compressed payload data.
    Decompression,     ///< Decompressing and validating an item.
    Extraction,        ///< Committing extracted filesystem content.
    Compression,       ///< Compressing a new item.
    Finalization,      ///< Writing directory/end records or committing an archive.
    DirectoryTraversal ///< Traversing source files for `addDirectory()`.
};

}
