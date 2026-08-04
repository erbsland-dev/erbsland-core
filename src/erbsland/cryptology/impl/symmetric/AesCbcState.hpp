// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../algorithm/aes/AesBlockCipher.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../symmetric/SymmetricEncryptionType.hpp"

#include <cstddef>
#include <memory>

namespace erbsland::cryptology::impl {

/// Shared streaming state for the two legacy AES-256-CBC constructions.
/// CBC chaining follows NIST SP 800-38A, Section 6.2; padding behavior is selected by the construction type.
/// @tested{AesCbcTest AesCbcFullTest}
class AesCbcState final {
public:
    /// The fixed AES block type.
    using Block = AesBlockCipher::Block;

public:
    /// Create AES-256-CBC state.
    /// @param type The random-fill or ISO/IEC 9797-1 method 2 construction.
    /// @param key The validated 256-bit AES key.
    /// @param iv The validated 128-bit initialization vector.
    AesCbcState(SymmetricEncryptionType type, mem::ConstByteSpan key, mem::ConstByteSpan iv);

    /// Securely releases the AES-CBC state.
    ~AesCbcState() noexcept;

    // defaults/deletions
    AesCbcState(const AesCbcState &) = delete;
    AesCbcState(AesCbcState &&) = delete;
    auto operator=(const AesCbcState &) -> AesCbcState & = delete;
    auto operator=(AesCbcState &&) -> AesCbcState & = delete;

public:
    /// Encrypt complete CBC blocks and retain a partial plaintext block.
    [[nodiscard]] auto encrypt(mem::ConstByteSpan data) -> mem::ByteBlock;
    /// Finalize encryption with the selected padding rule.
    [[nodiscard]] auto finalizeEncryption() -> mem::ByteBlock;
    /// Decrypt complete CBC blocks, retaining the last block for ISO padding validation.
    [[nodiscard]] auto decrypt(mem::ConstByteSpan data) -> mem::ByteBlock;
    /// Finalize decryption and validate or retain padding according to the construction.
    [[nodiscard]] auto finalizeDecryption() -> mem::ByteBlock;
    /// Securely erase the AES key, chaining values, pending blocks, and partial input.
    void secureErase() noexcept;

private:
    /// Encrypt and append one block using SP 800-38A CBC Equation Cj = AES(Pj XOR Cj-1).
    void encryptBlock(const Block &plaintext, mem::ByteBlockEditor &output);
    /// Decrypt and append one block using SP 800-38A CBC Equation Pj = AES^-1(Cj) XOR Cj-1.
    void decryptBlock(const Block &ciphertext, mem::ByteBlockEditor &output);
    /// Accept one complete ciphertext block, retaining it when ISO padding is enabled.
    void acceptCiphertextBlock(const Block &ciphertext, mem::ByteBlockEditor &output);
    /// Reset the partial block to a zero-filled empty state.
    void clearPartial() noexcept;
    /// Report the single generic CBC finalization error.
    [[noreturn]] static void throwFinalizationError();

private:
    SymmetricEncryptionType _type;           ///< Selected legacy padding construction.
    std::unique_ptr<AesBlockCipher> _cipher; ///< Automatically selected AES backend.
    Block _chain;                            ///< Previous ciphertext block, initially the IV.
    Block _partial;                          ///< Partial plaintext or ciphertext block.
    Block _pendingCiphertext;                ///< Final ciphertext block retained for ISO validation.
    std::size_t _partialLength{};            ///< Bytes stored in `_partial`.
    bool _hasPendingCiphertext{};            ///< Whether `_pendingCiphertext` is occupied.
};

}
