// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/core/Definitions.hpp>
#include <erbsland/cryptology/impl/algorithm/rsa_signature/RsaSigner.hpp>
#include <erbsland/cryptology/keys/PublicKey.hpp>
#include <erbsland/MakeOneNamespace.hpp>
#include <erbsland/mem/ByteBlock.hpp>

namespace app::constant_time {

/// Pinned compiled RSA key and validated arithmetic components.
/// @tested{ConstantTimeResourcesTest}
class RsaContext final {
public:
    /// Read the embedded fixture and validate its key components outside measurement.
    RsaContext();
    /// Get the PKCS1 private-key components.
    [[nodiscard]] auto privateKey() const noexcept -> const el::ByteBlock & { return _privateKey; }
    /// Get the matching public key.
    [[nodiscard]] auto publicKey() const noexcept -> const el::PublicKey & { return _publicKey; }
    /// Get the decoded fixed-width arithmetic context.
    [[nodiscard]] auto components() const noexcept -> const erbsland::cryptology::impl::rsa_signer::PrivateKeyData & {
        return _components;
    }

private:
    el::ByteBlock _privateKey;                                          ///< Valid PKCS1 private components.
    el::PublicKey _publicKey;                                           ///< Matching verification key.
    erbsland::cryptology::impl::rsa_signer::PrivateKeyData _components; ///< Prepared fixed-width modulus.
};

}
