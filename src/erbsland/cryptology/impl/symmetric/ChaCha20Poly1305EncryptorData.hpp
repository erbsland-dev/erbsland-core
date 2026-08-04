// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ChaCha20Poly1305State.hpp"
#include "SymmetricEncryptorData.hpp"

namespace erbsland::cryptology::impl {

/// RFC 8439 ChaCha20-Poly1305 encryption worker.
/// @tested{ChaCha20Poly1305Test ChaCha20Poly1305FullTest}
class ChaCha20Poly1305EncryptorData final : public SymmetricEncryptorData {
public:
    /// Create a ChaCha20-Poly1305 encryption worker.
    /// @param key The validated 256-bit key.
    /// @param nonce The validated 96-bit nonce.
    ChaCha20Poly1305EncryptorData(mem::ConstByteSpan key, mem::ConstByteSpan nonce);

    ~ChaCha20Poly1305EncryptorData() noexcept override;

    // defaults/deletions
    ChaCha20Poly1305EncryptorData(const ChaCha20Poly1305EncryptorData &) = delete;
    ChaCha20Poly1305EncryptorData(ChaCha20Poly1305EncryptorData &&) = delete;
    auto operator=(const ChaCha20Poly1305EncryptorData &) -> ChaCha20Poly1305EncryptorData & = delete;
    auto operator=(ChaCha20Poly1305EncryptorData &&) -> ChaCha20Poly1305EncryptorData & = delete;

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
    ChaCha20Poly1305State _state;      ///< Shared RFC 8439 transformation and framing state.
    ChaCha20Poly1305State::Tag _tag{}; ///< Cached tag after finalization.
};

}
