// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../algorithm/chacha20/ChaCha20Backend.hpp"
#include "../algorithm/chacha20/Poly1305.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace erbsland::cryptology::impl {

/// Shared streaming state for the RFC 8439 ChaCha20-Poly1305 AEAD construction.
/// This type owns framing, length accounting, counter advancement, streaming tails, and authentication.
/// @tested{ChaCha20Poly1305Test ChaCha20Poly1305FullTest}
class ChaCha20Poly1305State final {
#ifdef ERBSLAND_UNITTEST_BUILD
    friend class ChaCha20Poly1305StateTestAccess;
#endif

public:
    /// The fixed 128-bit authentication tag.
    /// @tested{ChaCha20Poly1305Test ChaCha20Poly1305FullTest}
    using Tag = Poly1305::Tag;

public:
    /// Create state for a 256-bit key and 96-bit nonce.
    /// @param key The validated key bytes.
    /// @param nonce The validated nonce bytes.
    ChaCha20Poly1305State(mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    /// Securely erase ChaCha20-Poly1305 state.
    ~ChaCha20Poly1305State() noexcept;

    // defaults/deletions
    ChaCha20Poly1305State(const ChaCha20Poly1305State &) = delete;
    ChaCha20Poly1305State(ChaCha20Poly1305State &&) = delete;
    auto operator=(const ChaCha20Poly1305State &) -> ChaCha20Poly1305State & = delete;
    auto operator=(ChaCha20Poly1305State &&) -> ChaCha20Poly1305State & = delete;

public:
    /// Add associated data as specified by RFC 8439 Section 2.8.
    void addAuthenticatedData(mem::ConstByteSpan data);
    /// Transform payload bytes and authenticate the ciphertext.
    /// @param data Plaintext for encryption or ciphertext for decryption.
    /// @param encrypting `true` to authenticate output, `false` to authenticate input.
    /// @return The transformed bytes.
    [[nodiscard]] auto transform(mem::ConstByteSpan data, bool encrypting) -> mem::ByteBlock;
    /// Finalize the RFC 8439 Section 2.8 framing and calculate the tag.
    [[nodiscard]] auto finalizeTag() noexcept -> Tag;
    /// Securely erase the key stream, authenticator, counters, and lengths.
    void secureErase() noexcept;

private:
    /// Close the AAD section by adding RFC 8439 `pad16(AAD)` exactly once.
    void finishAuthenticatedData() noexcept;
    /// Add enough zero bytes to align a section to a 16-byte boundary.
    void addPadding(uint64_t length) noexcept;
    /// Refill the stream cache with one scalar block or one four-block backend batch.
    void generateKeyStream(std::size_t remainingInput) noexcept;

private:
    static constexpr uint64_t maximumPayloadLength = uint64_t{0xffffffffU} * 64U; ///< RFC 8439 counter limit.

    std::unique_ptr<ChaCha20Backend> _chacha20; ///< Independently selected ChaCha20 backend.
    std::unique_ptr<Poly1305> _poly1305;        ///< Independently selected Poly1305 backend.
    ChaCha20Backend::Batch _keyStream;          ///< One or four cached ChaCha20 blocks.
    std::size_t _keyStreamOffset{};             ///< Next byte in `_keyStream`.
    std::size_t _keyStreamLength{};             ///< Number of valid cached bytes.
    uint64_t _nextCounter{1U};                  ///< Next payload block counter, held wide to detect exhaustion.
    uint64_t _authenticatedDataLength{};        ///< Accepted AAD byte count.
    uint64_t _payloadLength{};                  ///< Accepted ciphertext byte count.
    bool _authenticatedDataFinished{};          ///< Whether `pad16(AAD)` was submitted.
};

}
