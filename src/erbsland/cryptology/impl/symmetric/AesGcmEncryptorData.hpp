// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AesGcmState.hpp"
#include "SymmetricEncryptorData.hpp"

namespace erbsland::cryptology::impl {

/// AES-128-GCM and AES-256-GCM encryption worker.
/// Streaming and authentication follow NIST SP 800-38D, Section 7.1.
/// @tested{AesGcmTest AesGcmFullTest}
class AesGcmEncryptorData final : public SymmetricEncryptorData {
public:
    /// Create an AES-GCM encryption worker.
    /// @param type `Aes128Gcm` or `Aes256Gcm`.
    /// @param key The validated AES key bytes.
    /// @param nonce The validated 96-bit nonce.
    AesGcmEncryptorData(SymmetricEncryptionType type, mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    ~AesGcmEncryptorData() noexcept override;

    // defaults/deletions
    AesGcmEncryptorData(const AesGcmEncryptorData &) = delete;
    AesGcmEncryptorData(AesGcmEncryptorData &&) = delete;
    auto operator=(const AesGcmEncryptorData &) -> AesGcmEncryptorData & = delete;
    auto operator=(AesGcmEncryptorData &&) -> AesGcmEncryptorData & = delete;

public:
    /// @copydoc SymmetricEncryptorData::type()
    [[nodiscard]] auto type() const noexcept -> SymmetricEncryptionType override;
    /// @copydoc SymmetricEncryptorData::addAuthenticatedData()
    void addAuthenticatedData(mem::ConstByteSpan data) override;
    /// @copydoc SymmetricEncryptorData::encrypt()
    [[nodiscard]] auto encrypt(mem::ConstByteSpan data) -> mem::ByteBlock override;
    /// @copydoc SymmetricEncryptorData::finalize()
    [[nodiscard]] auto finalize() -> mem::ByteBlock override;
    /// @copydoc SymmetricEncryptorData::tag()
    [[nodiscard]] auto tag() const -> SymmetricTag override;
    /// @copydoc SymmetricEncryptorData::secureErase()
    void secureErase() noexcept override;

private:
    SymmetricEncryptionType _type; ///< AES key-width construction.
    AesGcmState _state;            ///< Shared GCM transformation and authentication state.
    AesGcmState::Block _tag;       ///< Cached tag after finalization.
};

}
