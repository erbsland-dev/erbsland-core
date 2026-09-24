// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::compression {

/// A machine-readable byte-compression failure reason.
enum class CompressionErrorReason : uint8_t {
    Cancelled,                  ///< A progress callback cancelled the transfer.
    Timeout,                    ///< A stream operation timed out.
    MalformedData,              ///< The compressed payload is malformed or truncated.
    LengthMismatch,             ///< Encoded and expected lengths do not match.
    UnsupportedAlgorithm,       ///< The selected or encoded algorithm is unsupported.
    AlgorithmMismatch,          ///< The encoded algorithm differs from the configured algorithm.
    UnsupportedFeature,         ///< The payload uses a recognized but unsupported codec feature.
    UnsupportedEnvelopeVersion, ///< The envelope version is unsupported.
};

}
