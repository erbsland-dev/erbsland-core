// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChaCha20Poly1305State.hpp"
#include "SymmetricDecryptorData.hpp"

namespace erbsland::cryptology::impl {

/// RFC 8439 ChaCha20-Poly1305 decryption and authentication worker.
/// Plaintext is returned incrementally before final authentication, matching the existing AEAD facade contract.
/// @tested{ChaCha20Poly1305Test ChaCha20Poly1305FullTest}
class ChaCha20Poly1305DecryptorData final : public SymmetricDecryptorData {
public:
    /// Create a ChaCha20-Poly1305 decryption worker.
    /// @param key The validated 256-bit key.
    /// @param nonce The validated 96-bit nonce.
    ChaCha20Poly1305DecryptorData(mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    ~ChaCha20Poly1305DecryptorData() noexcept override;

    // defaults/deletions
    ChaCha20Poly1305DecryptorData(const ChaCha20Poly1305DecryptorData &) = delete;
    ChaCha20Poly1305DecryptorData(ChaCha20Poly1305DecryptorData &&) = delete;
    auto operator=(const ChaCha20Poly1305DecryptorData &) -> ChaCha20Poly1305DecryptorData & = delete;
    auto operator=(ChaCha20Poly1305DecryptorData &&) -> ChaCha20Poly1305DecryptorData & = delete;

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
    ChaCha20Poly1305State _state;          ///< Shared RFC 8439 transformation and framing state.
    ChaCha20Poly1305State::Tag _actualTag; ///< Calculated tag retained until erasure.
};

}
