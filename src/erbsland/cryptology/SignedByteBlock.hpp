// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SignedByteBlock_fwd.hpp"

#include "keys/PublicKey_fwd.hpp"
#include "keys/SigningPrivateKey_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../mem/ByteSpan.hpp"
#include "../text/String.hpp"
#include "../unit/ByteCount.hpp"
#include "../unit/ByteIndex.hpp"

#include <optional>

namespace erbsland::cryptology {

/// A canonical, self-contained signed data record with purpose separation.
/// Parsed payload bytes remain unavailable until signature and purpose verification succeeds. The optional key ID is
/// only an untrusted lookup hint before verification.
/// @seedoc{/reference/cryptology/signed_byte_blocks}
/// @tested{SignedByteBlockTest}
class SignedByteBlock final {
public:
    /// Create an empty placeholder.
    SignedByteBlock() = default;

    // defaults
    ~SignedByteBlock() = default;
    SignedByteBlock(const SignedByteBlock &) = default;
    SignedByteBlock(SignedByteBlock &&) noexcept = default;
    auto operator=(const SignedByteBlock &) -> SignedByteBlock & = default;
    auto operator=(SignedByteBlock &&) noexcept -> SignedByteBlock & = default;

public: // main operations
    /// Verify the complete signed record and return its payload only on success.
    /// This signed record must not be an empty placeholder.
    /// @param key The expected Ed25519 public key.
    /// @param expectedPurpose The exact non-empty purpose chosen by the application.
    /// @return The authenticated payload, or no value for a mismatch.
    /// @throws CryptologyError If cryptographic verification fails internally.
    [[nodiscard]] auto verify(const PublicKey &key, const text::String &expectedPurpose) const
        -> std::optional<mem::ByteBlock>;

public: // tests and accessors
    /// Test whether this object contains a parsed or freshly signed record.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _encoded.isEmpty(); }
    /// Get the untrusted key ID hint for key lookup; verify before trusting it.
    [[nodiscard]] auto untrustedKeyIdHint() const noexcept -> const std::optional<text::String> & { return _keyId; }

public: // conversion
    /// Return the exact canonical binary record.
    [[nodiscard]] auto toByteBlock() const noexcept -> mem::ByteBlock { return _encoded; }
    /// Parse a binary record within an explicit payload limit.
    /// @param encoded The encoded signed record.
    /// @param maximumPayload The maximum accepted payload length.
    /// @return The parsed record, or an empty placeholder if malformed, over the limit, or the limit is infinite.
    [[nodiscard]] static auto fromByteBlock(mem::ConstByteSpan encoded, unit::ByteCount maximumPayload) noexcept
        -> SignedByteBlock;
    /// Parse a binary record within an explicit payload limit.
    /// @param encoded The encoded signed record.
    /// @param maximumPayload The maximum accepted payload length. Must be finite.
    /// @throws err::ParseError If malformed, noncanonical, over the payload limit, or using an unsupported format.
    /// @throws text::EncodingError If a text field contains invalid UTF-8.
    [[nodiscard]] static auto fromByteBlockOrThrow(mem::ConstByteSpan encoded, unit::ByteCount maximumPayload)
        -> SignedByteBlock;

public: // factories
    /// Sign a self-contained record with Ed25519.
    /// @param key A non-empty Ed25519 signing key.
    /// @param payload The data to embed and authenticate.
    /// @param purpose A non-empty, valid UTF-8 purpose of at most 255 bytes.
    /// @param keyId An optional `[_a-z0-9]{1,64}` lookup hint.
    /// @return The complete signed record.
    [[nodiscard]] static auto sign(
        const SigningPrivateKey &key,
        mem::ConstByteSpan payload,
        const text::String &purpose,
        std::optional<text::String> keyId = {}) -> SignedByteBlock;

private:
    /// Store a validated canonical record and its field positions.
    SignedByteBlock(
        mem::ByteBlock encoded,
        std::optional<text::String> keyId,
        text::String purpose,
        unit::ByteIndex payloadOffset,
        unit::ByteCount payloadLength) noexcept;

private:
    mem::ByteBlock _encoded;            ///< Complete canonical record.
    std::optional<text::String> _keyId; ///< Signed but unverified key-lookup hint.
    text::String _purpose;              ///< Signed but unverified purpose.
    unit::ByteIndex _payloadOffset{};   ///< Offset of embedded payload.
    unit::ByteCount _payloadLength{};   ///< Length of embedded payload.
};

}
