// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../HashAlgorithm.hpp"
#include "../../Hasher.hpp"

namespace erbsland::cryptology::impl {

/// Internal RFC 8446 Section 4.4.1 TLS 1.3 handshake transcript.
/// Callers provide complete handshake messages, including the one-byte type and three-byte length, but never record
/// headers. HelloRetryRequest transcript reconstruction is deliberately unavailable in the initial X25519-only client.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-4.4.1
/// @tested{Tls13KeyScheduleTest}
class Tls13Transcript final {
public:
    /// Create an empty transcript for the selected cipher-suite hash.
    /// @param algorithm The SHA-256 or SHA-384 transcript hash.
    explicit Tls13Transcript(HashAlgorithm algorithm);
    /// Erase message-dependent hash state.
    ~Tls13Transcript();

    // defaults/deletions
    Tls13Transcript(const Tls13Transcript &) = delete;
    Tls13Transcript(Tls13Transcript &&) noexcept = default;
    auto operator=(const Tls13Transcript &) -> Tls13Transcript & = delete;
    auto operator=(Tls13Transcript &&) noexcept -> Tls13Transcript & = default;

public:
    /// Add one exact complete handshake message.
    /// @param message The handshake header and body, with no record header.
    /// @throws err::ParameterError If the header is absent or its uint24 length is inconsistent.
    void update(mem::ConstByteSpan message);
    /// Snapshot the transcript hash without finalizing the live transcript.
    /// @return The current transcript hash.
    [[nodiscard]] auto hash() const -> mem::ByteBlock;
    /// Erase all message-dependent state and restore an empty transcript.
    void secureErase();

public: // accessors
    /// Get the selected transcript hash algorithm.
    [[nodiscard]] auto algorithm() const -> HashAlgorithm { return _hasher.algorithm(); }

private:
    Hasher _hasher; ///< Copy-on-write transcript hash state.
};

}
