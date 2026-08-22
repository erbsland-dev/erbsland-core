// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::mem {

/// A machine-readable byte-compression failure reason.
enum class ByteCompressionErrorReason : uint8_t {
    MalformedData,              ///< The compressed payload is malformed or truncated.
    LengthMismatch,             ///< Encoded and expected lengths do not match.
    UnsupportedAlgorithm,       ///< The selected or encoded algorithm is unsupported.
    UnsupportedEnvelopeVersion, ///< The envelope version is unsupported.
};

}
