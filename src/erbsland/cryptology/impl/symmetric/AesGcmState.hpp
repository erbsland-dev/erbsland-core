// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../algorithm/aes/AesBlockCipher.hpp"
#include "../algorithm/aes/GHash.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace erbsland::cryptology::impl {

/// Shared streaming state for AES-GCM encryption and decryption.
/// The construction follows NIST SP 800-38D, Sections 5.2.1, 6.2, 7.1, and 7.2.
/// @tested{AesGcmTest AesGcmFullTest}
class AesGcmState final {
public:
    /// The fixed 128-bit GCM block and authentication-tag type.
    using Block = AesBlockCipher::Block;

public:
    /// Create state for a 128-bit or 256-bit AES key and a 96-bit nonce.
    /// @param key The validated AES key.
    /// @param nonce The validated 96-bit nonce.
    AesGcmState(mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    /// Securely erase AES-GCM state.
    ~AesGcmState() noexcept;

    // defaults/deletions
    AesGcmState(const AesGcmState &) = delete;
    AesGcmState(AesGcmState &&) = delete;
    auto operator=(const AesGcmState &) -> AesGcmState & = delete;
    auto operator=(AesGcmState &&) -> AesGcmState & = delete;

public:
    /// Add authenticated associated data as specified by SP 800-38D, Section 7.1.
    void addAuthenticatedData(mem::ConstByteSpan data);
    /// Transform payload bytes with GCTR and authenticate ciphertext.
    /// @param data Plaintext for encryption or ciphertext for decryption.
    /// @param encrypting `true` to authenticate output, `false` to authenticate input.
    /// @return The transformed bytes.
    [[nodiscard]] auto transform(mem::ConstByteSpan data, bool encrypting) -> mem::ByteBlock;
    /// Finalize GHASH and calculate the 128-bit authentication tag.
    [[nodiscard]] auto finalizeTag() noexcept -> Block;
    /// Securely erase all keys, counters, hash state, and cached stream bytes.
    void secureErase() noexcept;

private:
    /// Increment the rightmost 32 counter bits as specified by SP 800-38D, Section 6.2.
    void incrementCounter() noexcept;
    /// Generate the next GCTR key-stream block.
    void generateKeyStreamBlock() noexcept;

private:
    static constexpr uint64_t maximumPayloadLength = (uint64_t{1U} << 36U) - 32U;          ///< SP 800-38D limit.
    static constexpr uint64_t maximumAuthenticatedDataLength = (uint64_t{1U} << 61U) - 1U; ///< Byte-aligned limit.

    std::unique_ptr<AesBlockCipher> _cipher; ///< Independently selected AES backend.
    std::unique_ptr<GHash> _gHash;           ///< Independently selected GHASH backend and state.
    Block _initialCounter;                   ///< J0 = nonce || 0x00000001.
    Block _counter;                          ///< Current inc32 counter value.
    Block _keyStream;                        ///< Current encrypted counter block.
    std::size_t _keyStreamOffset{16U};       ///< Next byte in `_keyStream`.
    uint64_t _authenticatedDataLength{};     ///< Accepted AAD bytes.
    uint64_t _payloadLength{};               ///< Accepted payload bytes.
};

}
