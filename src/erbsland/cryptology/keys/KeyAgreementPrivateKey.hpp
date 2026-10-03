// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyAgreementPrivateKey_fwd.hpp"
#include "KeyAgreementPublicKey.hpp"
#include "KeyAgreementSharedSecret.hpp"

#include "../protected_data/ProtectedByteBlock.hpp"

namespace erbsland::cryptology {

/// A move-only private key retained in application-protected storage.
/// @seedoc{/reference/cryptology/key_management}
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
    /// This private key must not be empty.
    /// @param peer The peer public key. Must not be empty and must use the same algorithm as this private key.
    /// @return The protected shared secret.
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
    /// This private key must not be empty.
    [[nodiscard]] auto publicKey() const -> KeyAgreementPublicKey;

public: // factories
    /// Generate a private key with the application secure random source.
    /// @param algorithm The key-agreement algorithm. Must be a valid supported algorithm.
    [[nodiscard]] static auto generate(KeyAgreementAlgorithm algorithm) -> KeyAgreementPrivateKey;
    /// Import exact private-key bytes into protected storage.
    /// The borrowed source remains owned by the caller and is not erased.
    /// @param algorithm The key-agreement algorithm. Must be a valid supported algorithm.
    /// @param data The exact encoded private key. Its length must match the selected algorithm.
    /// @throws err::OutOfRangeError If the input length does not match the selected algorithm.
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
