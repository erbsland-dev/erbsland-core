// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::network::impl {

/// A stable category for a strict HTTP/1.x codec failure.
enum class Http1FailureReason : std::uint8_t {
    None,                      ///< No failure occurred.
    MalformedStartLine,        ///< The request or status line is malformed.
    StartLineTooLong,          ///< The start line exceeds its configured limit.
    UnsupportedVersion,        ///< The HTTP version is not supported.
    MalformedField,            ///< A header or trailer field line is malformed.
    HeaderLimitExceeded,       ///< A header resource limit was exceeded.
    TrailerLimitExceeded,      ///< A trailer resource limit was exceeded.
    AmbiguousFraming,          ///< Content-Length or Transfer-Encoding is ambiguous.
    UnsupportedTransferCoding, ///< A transfer coding other than a single final chunked coding was used.
    MalformedChunk,            ///< Chunk syntax or delimiters are malformed.
    ChunkMetadataTooLong,      ///< A chunk-size and extension line exceeds its limit.
    ForbiddenTrailer,          ///< A recognized core field was received as a trailer.
    BodyLimitExceeded,         ///< The decoded body exceeds its configured limit.
    InputLimitExceeded,        ///< Retained input exceeds its configured queue limit.
    PrematureEndOfStream,      ///< EOF occurred before explicit framing completed.
    UnexpectedData,            ///< Bytes were supplied where HTTP framing disallows them.
    InvalidState,              ///< The codec operation is invalid for its lifecycle state.
};

}
