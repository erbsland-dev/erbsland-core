// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "SignedByteBlock_fwd.hpp"

#include "keys/PublicKey_fwd.hpp"
#include "keys/SigningPrivateKey_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../mem/ByteSpan.hpp"
#include "../text/String.hpp"
#include "../unit/ByteLength.hpp"

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
    /// @param key The expected Ed25519 public key.
    /// @param expectedPurpose The exact non-empty purpose chosen by the application.
    /// @return The authenticated payload, or no value for a mismatch.
    /// @throws err::LogicError If this is an empty placeholder.
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
    /// Parse a binary record within an explicit payload limit, returning an empty placeholder on malformed input.
    [[nodiscard]] static auto fromByteBlock(mem::ConstByteSpan encoded, unit::ByteLength maximumPayload) noexcept
        -> SignedByteBlock;
    /// Parse a binary record within an explicit payload limit.
    /// @throws err::ParseError If the record is malformed, noncanonical, or uses an unsupported version or algorithm.
    /// @throws err::OutOfRangeError If the payload exceeds the caller's limit.
    /// @throws err::ParameterError If `maximumPayload` is infinite.
    [[nodiscard]] static auto fromByteBlockOrThrow(mem::ConstByteSpan encoded, unit::ByteLength maximumPayload)
        -> SignedByteBlock;

public: // factories
    /// Sign a self-contained record with Ed25519.
    /// @param key An Ed25519 signing key.
    /// @param payload The data to embed and authenticate.
    /// @param purpose A non-empty, valid UTF-8 purpose of at most 255 bytes.
    /// @param keyId An optional `[_a-z0-9]{1,64}` lookup hint.
    /// @return The complete signed record.
    /// @throws err::ParameterError If any input is invalid or the key is not Ed25519.
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
        std::size_t payloadOffset,
        std::size_t payloadLength) noexcept;

private:
    mem::ByteBlock _encoded;            ///< Complete canonical record.
    std::optional<text::String> _keyId; ///< Signed but unverified key-lookup hint.
    text::String _purpose;              ///< Signed but unverified purpose.
    std::size_t _payloadOffset{};       ///< Offset of embedded payload.
    std::size_t _payloadLength{};       ///< Length of embedded payload.
};

}
