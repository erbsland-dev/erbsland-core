// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordContentType.hpp"
#include "TlsTrafficSecret.hpp"

#include "../impl/tls/TlsRecordState_fwd.hpp"
#include "../tls/TlsCipherSuite.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../unit/ByteLength.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A move-only TLS 1.3 record encryptor implementing RFC 8446 sections 5.2--5.5 and 7.2--7.3.
/// It constructs complete TLSCiphertext records, maintains one sending sequence, enforces key-usage limits, and keeps
/// traffic secrets protected outside scoped derivation. Padding length is explicit; no padding policy is implied.
/// @seedoc{/reference/cryptology/tls_record_protection}
/// @tested{TlsRecordProtectionTest}
class TlsRecordEncryptor final {
    friend class impl::TlsRecordTestAccess;

public:
    /// Create an empty record-encryptor placeholder.
    TlsRecordEncryptor() noexcept;
    /// Create RFC 8446 Sections 5.3 and 7.3 sending state by consuming a matching traffic secret.
    /// @throws err::ParameterError If the traffic secret does not match the suite hash.
    /// @throws CryptologyError If key derivation or protected storage fails.
    TlsRecordEncryptor(TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret);
    /// Securely erase record-protection state.
    ~TlsRecordEncryptor();

    // defaults/deletions
    TlsRecordEncryptor(const TlsRecordEncryptor &) = delete;
    TlsRecordEncryptor(TlsRecordEncryptor &&) noexcept;
    // defaults/deletions
    auto operator=(const TlsRecordEncryptor &) -> TlsRecordEncryptor & = delete;
    auto operator=(TlsRecordEncryptor &&) noexcept -> TlsRecordEncryptor &;

public:
    /// Protect one complete TLS 1.3 record under RFC 8446 Sections 5.2--5.5.
    /// @param type The authenticated inner content type.
    /// @param content The exact content bytes, at most 2^14 bytes.
    /// @param paddingLength The explicit number of zero padding octets.
    /// @return A complete TLSCiphertext header and encrypted body.
    /// @throws err::ParameterError If type or plaintext bounds are invalid.
    /// @throws TlsRecordError If key usage is exhausted.
    /// @throws CryptologyError If encryption fails.
    [[nodiscard]] auto protect(
        TlsRecordContentType type,
        mem::ConstByteSpan content,
        unit::ByteLength paddingLength = unit::ByteLength::zero()) -> mem::ByteBlock;
    /// Derive and install the next application traffic-secret generation under RFC 8446 Sections 4.6.3 and 7.2.
    /// The caller must invoke this only at the RFC 8446 section 4.6.3 protocol transition.
    void updateApplicationTrafficKeys();
    /// Securely erase all retained secret state and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no record-protection state is retained.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _state == nullptr; }
    /// Test the RFC 8446 Section 5.5 signal for sending KeyUpdate before more application data.
    [[nodiscard]] auto isKeyUpdateRequired() const noexcept -> bool;

public: // accessors
    /// Get the configured cipher suite.
    /// @throws err::LogicError If this encryptor is empty.
    [[nodiscard]] auto suite() const -> TlsCipherSuite;

private:
    std::unique_ptr<impl::TlsRecordState> _state; ///< Sending traffic-secret generation and counters.
};

}
