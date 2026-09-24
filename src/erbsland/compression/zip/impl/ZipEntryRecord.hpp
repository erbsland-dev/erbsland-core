// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionMethod.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../path/Path.hpp"
#include "../../../text/String.hpp"
#include "../../../time/DateTime.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../../unit/ByteLength.hpp"

#include <cstdint>

namespace erbsland::compression::zip::impl {

/// Validated metadata retained for one central-directory entry.
/// @tested{ZipArchiveTest}
struct ZipEntryRecord final {
    path::Path path;
    mem::ByteBlock name;
    text::String comment;
    time::DateTime modificationTime;
    CompressionMethod compressionMethod{CompressionMethod::Stored};
    uint16_t versionNeeded{};
    uint16_t flags{};
    uint16_t dosTime{};
    uint16_t dosDate{};
    uint32_t crc32{};
    unit::ByteLength compressedLength;
    unit::ByteLength uncompressedLength;
    unit::ByteIndex localHeaderOffset;
    bool directory{};
    bool zip64{};
    bool zip64Sizes{};
    mem::ByteBlock opaqueCentralExtra;
};

/// Local metadata and compressed bytes loaded for one selected entry.
/// @tested{ZipArchiveTest}
struct ZipEntryData final {
    mem::ByteBlock payload;
    mem::ByteBlock opaqueLocalExtra;
    unit::ByteIndex payloadOffset; ///< Validated payload start.
};

}
