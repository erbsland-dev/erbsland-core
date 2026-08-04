// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Tls13KeyScheduleTestAccess_fwd.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../unit/ByteLength.hpp"
#include "../../HashAlgorithm.hpp"
#include "../../keys/KeyAgreementSharedSecret.hpp"
#include "../../protected_data/ProtectedByteBlock.hpp"
#include "../../tls_record/TlsTrafficSecret.hpp"

namespace erbsland::cryptology::impl {

/// Internal RFC 8446 Section 7.1 non-PSK TLS 1.3 key schedule.
/// Secrets are derived in specification order. Retained secrets use `ProtectedByteBlock`; plaintext intermediates use
/// sensitive allocations with scope guards and are erased at the transition where they cease to be required.
/// PSK binders, early traffic, and 0-RTT are deliberately not represented by this initial client schedule.
/// Specification: https://www.rfc-editor.org/rfc/rfc8446.html#section-7.1
/// @tested{Tls13KeyScheduleTest}
class Tls13KeySchedule final {
    friend class Tls13KeyScheduleTestAccess;

private:
    /// One-way schedule phase used to enforce RFC 8446 derivation order.
    enum class Phase {
        Early,       ///< The zero-PSK early secret exists.
        Handshake,   ///< Handshake and master secrets exist.
        Application, ///< Application and exporter secrets exist.
        Finished,    ///< The resumption master was derived and discarded.
        Erased,      ///< All retained secret state was erased.
    };

public:
    /// Create the zero-PSK early-secret stage for SHA-256 or SHA-384.
    /// @param algorithm The cipher-suite HKDF and transcript hash.
    explicit Tls13KeySchedule(HashAlgorithm algorithm);
    /// Erase every retained secret.
    ~Tls13KeySchedule();

    // defaults/deletions
    Tls13KeySchedule(const Tls13KeySchedule &) = delete;
    Tls13KeySchedule(Tls13KeySchedule &&) = delete;
    auto operator=(const Tls13KeySchedule &) -> Tls13KeySchedule & = delete;
    auto operator=(Tls13KeySchedule &&) -> Tls13KeySchedule & = delete;

public:
    /// Consume an X25519 shared secret and derive handshake traffic and master secrets.
    /// The shared secret is erased on success and on every failure path.
    /// @param sharedSecret The protected X25519 shared secret.
    /// @param helloTranscriptHash Transcript-Hash(ClientHello...ServerHello).
    /// @throws err::LogicError If this schedule is not in the early phase.
    void initializeHandshake(KeyAgreementSharedSecret &&sharedSecret, mem::ConstByteSpan helloTranscriptHash);
    /// Derive and retain application traffic and exporter master secrets.
    /// @param serverFinishedTranscriptHash Transcript-Hash through the server Finished message.
    /// @throws err::LogicError If this schedule is not in the handshake phase.
    void initializeApplication(mem::ConstByteSpan serverFinishedTranscriptHash);
    /// Calculate one Finished verify_data value.
    /// @param forClient `true` for client Finished, `false` for server Finished.
    /// @param transcriptHash Transcript-Hash excluding the Finished message being calculated.
    /// @return Sensitive digest-sized verify_data.
    /// @throws err::LogicError If handshake traffic secrets are unavailable.
    [[nodiscard]] auto finishedVerifyData(bool forClient, mem::ConstByteSpan transcriptHash) const -> mem::ByteBlock;
    /// Verify one complete Finished value without content-dependent comparison.
    /// @param forClient `true` for client Finished, `false` for server Finished.
    /// @param transcriptHash Transcript-Hash excluding the Finished message being verified.
    /// @param verifyData The peer's exact digest-sized verify_data.
    /// @return `true` if the complete value matches.
    [[nodiscard]] auto verifyFinished(
        bool forClient, mem::ConstByteSpan transcriptHash, mem::ConstByteSpan verifyData) const -> bool;
    /// Copy one protected handshake traffic secret into the record-layer type.
    /// @param forClient `true` for the client write direction, `false` for the server write direction.
    /// @return A separately protected traffic secret.
    [[nodiscard]] auto handshakeTrafficSecret(bool forClient) const -> TlsTrafficSecret;
    /// Copy one protected application traffic secret into the record-layer type.
    /// @param forClient `true` for the client write direction, `false` for the server write direction.
    /// @return A separately protected traffic secret.
    [[nodiscard]] auto applicationTrafficSecret(bool forClient) const -> TlsTrafficSecret;
    /// Export keying material following RFC 8446 Section 7.5.
    /// @param label The application label without the mandatory TLS prefix.
    /// @param context The application context whose hash is bound to the export.
    /// @param outputLength The requested output length.
    /// @return Sensitive exported keying material.
    /// @throws err::LogicError If the exporter master secret is unavailable.
    [[nodiscard]] auto exportKey(
        mem::ConstByteSpan label, mem::ConstByteSpan context, unit::ByteLength outputLength) const -> mem::ByteBlock;
    /// Derive and immediately erase the unused resumption master secret.
    /// Session tickets and PSK resumption are deliberately deferred.
    /// @param clientFinishedTranscriptHash Transcript-Hash through the client Finished message.
    /// @throws err::LogicError If application secrets have not been initialized.
    void discardResumptionMaster(mem::ConstByteSpan clientFinishedTranscriptHash);
    /// Securely erase all retained schedule state.
    void secureErase() noexcept;

public: // tests
    /// Test whether all retained secret state has been erased.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _phase == Phase::Erased; }

public: // accessors
    /// Get the bound HKDF and transcript hash.
    [[nodiscard]] auto algorithm() const noexcept -> HashAlgorithm { return _algorithm; }

private:
    /// Protect secret bytes and erase the plaintext allocation.
    static void protectAndErase(ProtectedByteBlock &destination, mem::ByteBlock &plaintext);
    /// Copy one protected secret into a record-layer traffic secret.
    [[nodiscard]] auto copyTrafficSecret(const ProtectedByteBlock &source) const -> TlsTrafficSecret;
    /// Calculate Finished from one protected base traffic secret.
    [[nodiscard]] auto finishedFrom(const ProtectedByteBlock &baseSecret, mem::ConstByteSpan transcriptHash) const
        -> mem::ByteBlock;

private:
    HashAlgorithm _algorithm;                    ///< Cipher-suite HKDF and transcript hash.
    Phase _phase{Phase::Early};                  ///< Enforced one-way schedule phase.
    ProtectedByteBlock _earlySecret;             ///< Zero-PSK early secret until handshake initialization.
    ProtectedByteBlock _clientHandshakeSecret;   ///< Client handshake traffic secret.
    ProtectedByteBlock _serverHandshakeSecret;   ///< Server handshake traffic secret.
    ProtectedByteBlock _masterSecret;            ///< Master secret retained until client Finished.
    ProtectedByteBlock _clientApplicationSecret; ///< Client application traffic secret generation zero.
    ProtectedByteBlock _serverApplicationSecret; ///< Server application traffic secret generation zero.
    ProtectedByteBlock _exporterMasterSecret;    ///< Exporter master secret retained for RFC 8446 Section 7.5.
};

}
