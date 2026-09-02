// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../HashAlgorithm.hpp"
#include "../impl/tls/TlsRecordState_fwd.hpp"
#include "../protected_data/ProtectedByteBlock.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"

namespace erbsland::cryptology {

/// A move-only TLS 1.3 traffic secret retained in application-protected storage.
/// The secret length is the selected SHA-256 or SHA-384 digest length required by RFC 8446 sections 7.1--7.3. There is
/// no public plaintext accessor; record protection resolves the secret only for scoped derivation and erasure.
/// @seedoc{/reference/cryptology/tls}
/// @tested{TlsRecordProtectionTest}
class TlsTrafficSecret final {
    friend class impl::TlsRecordState;
    friend class impl::TlsRecordTestAccess;

public:
    /// Create an empty traffic-secret placeholder.
    TlsTrafficSecret() noexcept = default;
    /// Securely erase the protected envelope.
    ~TlsTrafficSecret();

    // defaults/deletions
    TlsTrafficSecret(const TlsTrafficSecret &) = delete;
    TlsTrafficSecret(TlsTrafficSecret &&) noexcept = default;
    auto operator=(const TlsTrafficSecret &) -> TlsTrafficSecret & = delete;
    auto operator=(TlsTrafficSecret &&) noexcept -> TlsTrafficSecret & = default;

public:
    /// Securely erase the protected envelope and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no traffic secret is stored.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _data.isEmpty(); }

public: // accessors
    /// Get the traffic-secret hash algorithm.
    [[nodiscard]] auto hashAlgorithm() const noexcept -> HashAlgorithm { return _hashAlgorithm; }
    /// Get the plaintext secret length without resolving it.
    [[nodiscard]] auto byteLength() const noexcept -> unit::ByteLength { return _data.byteLength(); }

public: // factories
    /// Protect a copy of RFC 8446 Sections 7.1--7.3 exact digest-sized traffic-secret bytes.
    /// The caller retains ownership and erasure responsibility for the borrowed bytes.
    /// @throws err::ParameterError If the algorithm or length is unsupported.
    [[nodiscard]] static auto fromBytes(HashAlgorithm algorithm, mem::ConstByteSpan data) -> TlsTrafficSecret;
    /// Protect and then erase an owning RFC 8446 Sections 7.1--7.3 exact digest-sized traffic-secret block.
    /// @throws err::ParameterError If the algorithm or length is unsupported.
    [[nodiscard]] static auto fromBytes(HashAlgorithm algorithm, mem::ByteBlock &&data) -> TlsTrafficSecret;

private:
    /// Store a validated traffic secret in protected storage.
    TlsTrafficSecret(HashAlgorithm algorithm, mem::ConstByteSpan data);
    /// Validate the supported hash and exact digest length.
    static void validate(HashAlgorithm algorithm, mem::ConstByteSpan data);

private:
    HashAlgorithm _hashAlgorithm{HashAlgorithm::Sha2_256}; ///< SHA-2 algorithm bound to the secret.
    ProtectedByteBlock _data;                              ///< Application-protected traffic-secret bytes.
};

}
