// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyAgreementPrivateKey_fwd.hpp"
#include "KeyAgreementPublicKey.hpp"
#include "KeyAgreementSharedSecret.hpp"

#include "../protected_data/ProtectedByteBlock.hpp"

namespace erbsland::cryptology {

/// A move-only private key retained in application-protected storage.
/// @seedoc{/reference/cryptology/key_agreement}
/// @tested{KeyAgreementTest}
class KeyAgreementPrivateKey final {
public:
    /// Create an empty private-key placeholder.
    KeyAgreementPrivateKey() noexcept = default;
    /// Securely erase the protected private-key envelope.
    ~KeyAgreementPrivateKey();

    // defaults/deletions
    KeyAgreementPrivateKey(const KeyAgreementPrivateKey &) = delete;
    KeyAgreementPrivateKey(KeyAgreementPrivateKey &&) noexcept = default;
    auto operator=(const KeyAgreementPrivateKey &) -> KeyAgreementPrivateKey & = delete;
    auto operator=(KeyAgreementPrivateKey &&) noexcept -> KeyAgreementPrivateKey & = default;

public:
    /// Agree with a peer public key.
    /// @return The protected shared secret.
    /// @throws err::LogicError If this private key or the peer key is empty.
    /// @throws err::ParameterError If the algorithms do not match.
    /// @throws CryptologyError If agreement produces the forbidden all-zero result.
    [[nodiscard]] auto agree(const KeyAgreementPublicKey &peer) const -> KeyAgreementSharedSecret;
    /// Securely erase private material and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no private key is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _privateData.isEmpty(); }

public: // accessors
    /// Get the key-agreement algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> KeyAgreementAlgorithm { return _algorithm; }
    /// Get the cached public key without decrypting private material.
    /// @throws err::LogicError If this private key is empty.
    [[nodiscard]] auto publicKey() const -> KeyAgreementPublicKey;

public: // factories
    /// Generate a private key with the application secure random source.
    /// @throws err::ParameterError If the algorithm is invalid.
    [[nodiscard]] static auto generate(KeyAgreementAlgorithm algorithm) -> KeyAgreementPrivateKey;
    /// Import exact private-key bytes into protected storage.
    /// The borrowed source remains owned by the caller and is not erased.
    /// @throws err::ParameterError If the algorithm or private-key length is invalid.
    [[nodiscard]] static auto fromBytes(KeyAgreementAlgorithm algorithm, mem::ConstByteSpan data)
        -> KeyAgreementPrivateKey;

private:
    /// Create a private key after its public key has been calculated.
    KeyAgreementPrivateKey(
        KeyAgreementAlgorithm algorithm, mem::ConstByteSpan privateData, KeyAgreementPublicKey publicKey);

private:
    KeyAgreementAlgorithm _algorithm; ///< Key-agreement algorithm.
    ProtectedByteBlock _privateData;  ///< Protected private-key bytes.
    KeyAgreementPublicKey _publicKey; ///< Cached public key.
};

}
