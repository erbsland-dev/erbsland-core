// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordState_fwd.hpp"

#include "../../../mem/ByteBlock.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../symmetric/SymmetricKey.hpp"
#include "../../symmetric/SymmetricNonce.hpp"
#include "../../tls/TlsCipherSuite.hpp"
#include "../../tls_record/TlsTrafficSecret.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

/// Shared RFC 8446 traffic-secret generation and sequence state for one record direction.
/// HKDF label encoding follows section 7.1, traffic-key derivation section 7.3, nonce derivation section 5.3, key
/// limits section 5.5, and application-traffic updates section 7.2.
/// @tested{TlsRecordProtectionTest}
class TlsRecordState final {
    friend class TlsRecordTestAccess;

public:
    /// Create one traffic direction and derive its first write key and IV.
    TlsRecordState(TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret);
    /// Erase traffic secret, key, IV, and counters.
    ~TlsRecordState();

    // defaults/deletions
    TlsRecordState(const TlsRecordState &) = delete;
    TlsRecordState(TlsRecordState &&) = delete;
    auto operator=(const TlsRecordState &) -> TlsRecordState & = delete;
    auto operator=(TlsRecordState &&) -> TlsRecordState & = delete;

public:
    /// Derive the nonce for the current record sequence.
    [[nodiscard]] auto recordNonce() const -> SymmetricNonce;
    /// Require that another record can be processed under this generation.
    /// @throws TlsRecordError If the cipher or sequence limit is exhausted.
    void requireRecordAvailable() const;
    /// Advance counters after one successfully processed record.
    void recordSucceeded() noexcept;
    /// Derive and install the next application traffic generation.
    void updateApplicationTrafficKeys();
    /// Securely erase all retained state.
    void secureErase() noexcept;

public: // tests
    /// Test whether the proactive KeyUpdate threshold is reached.
    [[nodiscard]] auto isKeyUpdateRequired() const noexcept -> bool;

public: // accessors
    /// Get the configured TLS cipher suite.
    [[nodiscard]] auto suite() const noexcept -> TlsCipherSuite { return _suite; }
    /// Get the derived record key.
    [[nodiscard]] auto key() const noexcept -> const SymmetricKey & { return _key; }
    /// Get the current sequence number.
    [[nodiscard]] auto sequenceNumber() const noexcept -> uint64_t { return _sequenceNumber; }

private:
    /// Derive the write key and static IV from a traffic secret.
    void deriveKeyAndIv(const TlsTrafficSecret &secret, SymmetricKey &key, SymmetricNonce &staticIv) const;
    /// Derive the next application traffic secret.
    [[nodiscard]] auto deriveUpdatedTrafficSecret() const -> TlsTrafficSecret;

private:
    static constexpr uint64_t cAesUpdateRecordCount{uint64_t{1U} << 24U}; ///< Proactive AES-GCM update threshold.
    static constexpr uint64_t cAesMaximumRecordCount{23'726'566U};        ///< floor(2^24.5) AES-GCM record limit.

    TlsCipherSuite _suite;                                                ///< Bound AEAD and HKDF hash.
    TlsTrafficSecret _trafficSecret; ///< Protected current-generation traffic secret.
    SymmetricKey _key;               ///< Sensitive current-generation record key.
    SymmetricNonce _staticIv;        ///< Sensitive current-generation 96-bit static IV.
    uint64_t _sequenceNumber{0U};    ///< Next 64-bit record sequence number.
    uint64_t _recordCount{0U};       ///< Successful records under this generation.
};

}
