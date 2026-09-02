// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "KeyAgreementAlgorithm.hpp"
#include "KeyAgreementPrivateKey_fwd.hpp"

#include "../Hkdf_fwd.hpp"
#include "../protected_data/ProtectedByteBlock.hpp"

namespace erbsland::cryptology {

/// A move-only shared secret retained in application-protected storage.
/// @seedoc{/reference/cryptology/key_management}
/// @tested{KeyAgreementTest}
class KeyAgreementSharedSecret final {
    friend class Hkdf;
    friend class KeyAgreementPrivateKey;

public:
    /// Create an empty shared-secret placeholder.
    KeyAgreementSharedSecret() noexcept = default;
    /// Securely erase the protected envelope.
    ~KeyAgreementSharedSecret();

    // defaults/deletions
    KeyAgreementSharedSecret(const KeyAgreementSharedSecret &) = delete;
    KeyAgreementSharedSecret(KeyAgreementSharedSecret &&) noexcept = default;
    auto operator=(const KeyAgreementSharedSecret &) -> KeyAgreementSharedSecret & = delete;
    auto operator=(KeyAgreementSharedSecret &&) noexcept -> KeyAgreementSharedSecret & = default;

public:
    /// Securely erase the protected envelope and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no shared secret is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.isEmpty(); }

public: // accessors
    /// Get the key-agreement algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> KeyAgreementAlgorithm { return _algorithm; }
    /// Get the shared-secret length without decrypting it.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength { return _data.byteLength(); }

private:
    /// Store one shared secret in protected storage.
    KeyAgreementSharedSecret(KeyAgreementAlgorithm algorithm, mem::ConstByteSpan data);

private:
    KeyAgreementAlgorithm _algorithm; ///< Key-agreement algorithm.
    ProtectedByteBlock _data;         ///< Protected shared-secret bytes.
};

}
