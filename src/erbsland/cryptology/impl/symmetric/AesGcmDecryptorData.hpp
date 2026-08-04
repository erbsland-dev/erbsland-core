// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesGcmState.hpp"
#include "SymmetricDecryptorData.hpp"

namespace erbsland::cryptology::impl {

/// AES-128-GCM and AES-256-GCM decryption and authentication worker.
/// Streaming and authentication follow NIST SP 800-38D, Section 7.2.
/// @tested{AesGcmTest AesGcmFullTest}
class AesGcmDecryptorData final : public SymmetricDecryptorData {
public:
    /// Create an AES-GCM decryption worker.
    /// @param type `Aes128Gcm` or `Aes256Gcm`.
    /// @param key The validated AES key bytes.
    /// @param nonce The validated 96-bit nonce.
    AesGcmDecryptorData(SymmetricEncryptionType type, mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    ~AesGcmDecryptorData() noexcept override;

    // defaults/deletions
    AesGcmDecryptorData(const AesGcmDecryptorData &) = delete;
    AesGcmDecryptorData(AesGcmDecryptorData &&) = delete;
    auto operator=(const AesGcmDecryptorData &) -> AesGcmDecryptorData & = delete;
    auto operator=(AesGcmDecryptorData &&) -> AesGcmDecryptorData & = delete;

public:
    /// @copydoc SymmetricDecryptorData::type()
    [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override;
    /// @copydoc SymmetricDecryptorData::addAuthenticatedData()
    void addAuthenticatedData(mem::ConstByteSpan data) override;
    /// @copydoc SymmetricDecryptorData::decrypt()
    [[nodiscard]] auto decrypt(mem::ConstByteSpan data) -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::finalize(const SymmetricTag &)
    [[nodiscard]] auto finalize(const SymmetricTag &tag) -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::finalize()
    [[nodiscard]] auto finalize() -> mem::ByteBlock override;
    /// @copydoc SymmetricDecryptorData::secureErase()
    void secureErase() noexcept override;

private:
    SymmetricEncryptionType _type; ///< AES key-width construction.
    AesGcmState _state;            ///< Shared GCM transformation and authentication state.
    AesGcmState::Block _tag;       ///< Calculated tag retained until erasure.
};

}
