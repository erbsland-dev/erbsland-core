// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression::zip {

/// A machine-readable ZIP archive failure reason.
enum class ZipErrorReason : uint8_t {
    MalformedArchive,   ///< A record is truncated, inconsistent, or otherwise malformed.
    UnsupportedFeature, ///< A recognized ZIP feature is outside the supported profile.
    UnsupportedMethod,  ///< An entry uses an unsupported compression method.
    ResourceLimit,      ///< A configured archive, item, workspace, or output limit was exceeded.
    IntegrityFailure,   ///< An extracted length or CRC-32 does not match the directory metadata.
    UnsafePath,         ///< An entry path is non-portable or unsafe for extraction.
    PathFailure,        ///< A filesystem operation failed.
    StreamFailure,      ///< An archive stream failed or timed out.
    CodecFailure        ///< Compression or decompression failed.
};

}
