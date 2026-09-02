// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "TlsRecordPlaintext.hpp"
#include "TlsTrafficSecret.hpp"

#include "../impl/tls/TlsRecordState_fwd.hpp"
#include "../tls/TlsCipherSuite.hpp"

#include "../../mem/ByteSpan.hpp"

#include <memory>

namespace erbsland::cryptology {

/// A move-only TLS 1.3 record decryptor implementing RFC 8446 sections 5.2--5.5 and 7.2--7.3.
/// It authenticates one complete TLSCiphertext record before returning sensitive plaintext. Any malformed record,
/// authentication failure, overflow, or exhausted key makes this state terminal and erases all retained secrets.
/// @seedoc{/reference/cryptology/tls}
/// @tested{TlsRecordProtectionTest}
class TlsRecordDecryptor final {
    friend class impl::TlsRecordTestAccess;

public:
    /// Create an empty record-decryptor placeholder.
    TlsRecordDecryptor() noexcept;
    /// Create RFC 8446 Sections 5.3 and 7.3 receiving state by consuming a matching traffic secret.
    /// @throws err::ParameterError If the traffic secret does not match the suite hash.
    /// @throws CryptologyError If key derivation or protected storage fails.
    TlsRecordDecryptor(TlsCipherSuite suite, TlsTrafficSecret &&trafficSecret);
    /// Securely erase record-protection state.
    ~TlsRecordDecryptor();

    // defaults/deletions
    TlsRecordDecryptor(const TlsRecordDecryptor &) = delete;
    TlsRecordDecryptor(TlsRecordDecryptor &&) noexcept;
    // defaults/deletions
    auto operator=(const TlsRecordDecryptor &) -> TlsRecordDecryptor & = delete;
    auto operator=(TlsRecordDecryptor &&) noexcept -> TlsRecordDecryptor &;

public:
    /// Authenticate and deprotect one exact complete TLS 1.3 record under RFC 8446 Sections 5.2--5.5.
    /// @param record The complete five-byte header and encrypted body.
    /// @return Authenticated sensitive content and its inner content type.
    /// @throws TlsRecordError For malformed, unauthenticated, overflowing, or exhausted input.
    /// @throws CryptologyError If the cryptographic backend fails independently of peer authentication.
    [[nodiscard]] auto unprotect(mem::ConstByteSpan record) -> TlsRecordPlaintext;
    /// Derive and install the next application traffic-secret generation under RFC 8446 Sections 4.6.3 and 7.2.
    /// The caller must invoke this only after authenticating the RFC 8446 section 4.6.3 KeyUpdate message.
    void updateApplicationTrafficKeys();
    /// Securely erase all retained secret state and restore the empty state.
    void secureErase() noexcept;

public: // tests
    /// Test whether no record-protection state is retained.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _state == nullptr; }
    /// Test the RFC 8446 Section 5.5 signal that the peer should update before more application data.
    [[nodiscard]] auto isKeyUpdateRequired() const noexcept -> bool;

public: // accessors
    /// Get the configured cipher suite.
    /// @throws err::LogicError If this decryptor is empty.
    [[nodiscard]] auto suite() const -> TlsCipherSuite;

private:
    std::unique_ptr<impl::TlsRecordState> _state; ///< Receiving traffic-secret generation and counters.
};

}
