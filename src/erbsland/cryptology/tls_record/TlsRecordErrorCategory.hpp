// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// A stable TLS 1.3 record-protection failure category.
enum class TlsRecordErrorCategory : uint8_t {
    DecodeError,       ///< The complete record framing or encoded length is malformed.
    UnexpectedMessage, ///< The outer or authenticated inner record semantics are invalid.
    BadRecordMac,      ///< AEAD authentication or decryption failed.
    RecordOverflow,    ///< A ciphertext or authenticated plaintext exceeds its RFC bound.
    KeyUsageExhausted, ///< The sequence number or cipher-specific record limit is exhausted.
};

}
