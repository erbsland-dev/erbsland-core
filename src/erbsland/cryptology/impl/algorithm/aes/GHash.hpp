// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "GaloisMultiplier.hpp"

#include "../../../../mem/ByteSpan.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace erbsland::cryptology::impl {

/// Incremental GHASH state for associated data and ciphertext.
/// Block accumulation and final length encoding follow NIST SP 800-38D, Sections 6.4 and 7.1.
/// @tested{AesGcmTest AesGcmFullTest}
class GHash final {
public:
    /// Create GHASH with the AES-derived hash subkey.
    /// @param hashSubkey The value AES_K(0^128).
    /// @param multiplier The selected field multiplier.
    GHash(const GaloisMultiplier::Block &hashSubkey, std::unique_ptr<GaloisMultiplier> multiplier) noexcept;

    /// Securely erase the GHASH state.
    ~GHash() noexcept;

    // defaults/deletions
    GHash(const GHash &) = delete;
    GHash(GHash &&) = delete;
    auto operator=(const GHash &) -> GHash & = delete;
    auto operator=(GHash &&) -> GHash & = delete;

public:
    /// Add authenticated associated data before ciphertext.
    void addAuthenticatedData(mem::ConstByteSpan data) noexcept;
    /// Add ciphertext bytes, closing and zero-padding any partial associated-data block first.
    void addCiphertext(mem::ConstByteSpan data) noexcept;
    /// Finalize GHASH with zero padding and the encoded bit lengths.
    [[nodiscard]] auto finalize() noexcept -> GaloisMultiplier::Block;
    /// Securely erase the hash subkey, accumulator, partial block, and counters.
    void secureErase() noexcept;

private:
    /// Absorb bytes into the current GHASH section.
    void add(mem::ConstByteSpan data) noexcept;
    /// Zero-pad and process the current partial block.
    void finishPartialBlock() noexcept;
    /// XOR and multiply one complete block as defined by SP 800-38D, Equation 2.
    void processBlock(const GaloisMultiplier::Block &block) noexcept;

private:
    GaloisMultiplier::Block _hashSubkey;           ///< AES_K(0^128).
    GaloisMultiplier::Block _accumulator;          ///< Current GHASH output block.
    GaloisMultiplier::Block _partial;              ///< Incomplete AAD or ciphertext block.
    std::unique_ptr<GaloisMultiplier> _multiplier; ///< Selected polynomial multiplier.
    std::size_t _partialLength{};                  ///< Bytes currently stored in `_partial`.
    uint64_t _authenticatedDataLength{};           ///< Total associated-data length in bytes.
    uint64_t _ciphertextLength{};                  ///< Total ciphertext length in bytes.
    bool _ciphertextStarted{};                     ///< Whether AAD input has been closed.
    bool _finalized{};                             ///< Whether the length block was processed.
};

}
