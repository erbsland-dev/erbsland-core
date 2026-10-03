// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Tls13KeySchedule.hpp"

#include "Tls13Hkdf.hpp"

#include "../SecureEraseGuard.hpp"

#include "../../../err/LogicFailure.hpp"
#include "../../../mem/Byte.hpp"
#include "../../../mem/ByteArray.hpp"
#include "../../../mem/ByteBlock.hpp"
#include "../../../text/Literals.hpp"
#include "../../Hasher.hpp"
#include "../../Hkdf.hpp"
#include "../../Hmac.hpp"

#include <utility>

namespace erbsland::cryptology::impl {

using namespace err;
using namespace mem;
using namespace text::literals;

Tls13KeySchedule::Tls13KeySchedule(const HashAlgorithm algorithm) : _algorithm{algorithm} {
    const auto zero = ByteBlock{_algorithm.digestSize()};

    // RFC 8446 section 7.1: Early Secret = HKDF-Extract(0, 0) when no PSK is selected. Both zero values have
    // Hash.length bytes so the representation and lifetime of this deliberate non-secret input remain explicit.
    auto earlySecret = Hkdf{_algorithm}.extract(zero.span());
    protectAndErase(_earlySecret, earlySecret);
}

Tls13KeySchedule::~Tls13KeySchedule() {
    secureErase();
}

void Tls13KeySchedule::initializeHandshake(
    KeyAgreementSharedSecret &&sharedSecret, const ConstByteSpan helloTranscriptHash) {
    // Keep the received ECDHE secret's erasure visible at the ownership-transfer boundary, including exceptions.
    auto ownedSharedSecret = std::move(sharedSecret);
    const auto sharedSecretEraseGuard = SecureEraseGuard{ownedSharedSecret};
    if (_phase != Phase::Early) {
        throw LogicFailure{"The TLS key schedule is not in the early-secret phase."_el};
    }

    const auto tlsHkdf = Tls13Hkdf{_algorithm};
    auto emptyTranscriptHash = tlsHkdf.emptyHash();
    auto derivedEarly = ByteBlock{};
    auto handshakeSecret = ByteBlock{};
    auto clientHandshakeSecret = ByteBlock{};
    auto serverHandshakeSecret = ByteBlock{};
    auto derivedHandshake = ByteBlock{};
    auto masterSecret = ByteBlock{};
    const auto emptyTranscriptHashEraseGuard = SecureEraseGuard{emptyTranscriptHash};
    const auto derivedEarlyEraseGuard = SecureEraseGuard{derivedEarly};
    const auto handshakeSecretEraseGuard = SecureEraseGuard{handshakeSecret};
    const auto clientHandshakeSecretEraseGuard = SecureEraseGuard{clientHandshakeSecret};
    const auto serverHandshakeSecretEraseGuard = SecureEraseGuard{serverHandshakeSecret};
    const auto derivedHandshakeEraseGuard = SecureEraseGuard{derivedHandshake};
    const auto masterSecretEraseGuard = SecureEraseGuard{masterSecret};

    static constexpr auto cDerivedLabel =
        ByteArray<7U>{Byte{'d'}, Byte{'e'}, Byte{'r'}, Byte{'i'}, Byte{'v'}, Byte{'e'}, Byte{'d'}};
    static constexpr auto cClientHandshakeLabel = ByteArray<12U>{
        Byte{'c'},
        Byte{' '},
        Byte{'h'},
        Byte{'s'},
        Byte{' '},
        Byte{'t'},
        Byte{'r'},
        Byte{'a'},
        Byte{'f'},
        Byte{'f'},
        Byte{'i'},
        Byte{'c'}};
    static constexpr auto cServerHandshakeLabel = ByteArray<12U>{
        Byte{'s'},
        Byte{' '},
        Byte{'h'},
        Byte{'s'},
        Byte{' '},
        Byte{'t'},
        Byte{'r'},
        Byte{'a'},
        Byte{'f'},
        Byte{'f'},
        Byte{'i'},
        Byte{'c'}};

    _earlySecret.withUnprotectedData([&](const ConstByteSpan earlySecret) -> void {
        // RFC 8446 section 7.1: Derive-Secret(Early Secret, "derived", "") provides the handshake extract salt.
        derivedEarly = tlsHkdf.deriveSecret(earlySecret, cDerivedLabel.span(), emptyTranscriptHash.span());

        // RFC 8446 section 7.1: Handshake Secret = HKDF-Extract(Derived(Early), (EC)DHE).
        handshakeSecret = Hkdf{_algorithm}.extract(ownedSharedSecret, derivedEarly.span());
    });

    // RFC 8446 section 7.1: client_handshake_traffic_secret = Derive-Secret(Handshake, "c hs traffic", CH...SH).
    clientHandshakeSecret =
        tlsHkdf.deriveSecret(handshakeSecret.span(), cClientHandshakeLabel.span(), helloTranscriptHash);

    // RFC 8446 section 7.1: server_handshake_traffic_secret = Derive-Secret(Handshake, "s hs traffic", CH...SH).
    serverHandshakeSecret =
        tlsHkdf.deriveSecret(handshakeSecret.span(), cServerHandshakeLabel.span(), helloTranscriptHash);

    // RFC 8446 section 7.1: Derive-Secret(Handshake Secret, "derived", "") provides the master extract salt.
    derivedHandshake = tlsHkdf.deriveSecret(handshakeSecret.span(), cDerivedLabel.span(), emptyTranscriptHash.span());

    // RFC 8446 section 7.1: Master Secret = HKDF-Extract(Derived(Handshake), 0), with a Hash.length zero IKM.
    const auto zero = ByteBlock{_algorithm.digestSize()};
    masterSecret = Hkdf{_algorithm}.extract(zero.span(), derivedHandshake.span());

    // Protect every retained result before releasing any old schedule state. Plaintext guards erase all temporaries.
    auto protectedClientHandshake = ProtectedByteBlock{clientHandshakeSecret.span()};
    auto protectedServerHandshake = ProtectedByteBlock{serverHandshakeSecret.span()};
    auto protectedMaster = ProtectedByteBlock{masterSecret.span()};
    _clientHandshakeSecret = std::move(protectedClientHandshake);
    _serverHandshakeSecret = std::move(protectedServerHandshake);
    _masterSecret = std::move(protectedMaster);
    _earlySecret.secureErase();
    _phase = Phase::Handshake;
}

void Tls13KeySchedule::initializeApplication(const ConstByteSpan serverFinishedTranscriptHash) {
    if (_phase != Phase::Handshake) {
        throw LogicFailure{"The TLS key schedule is not in the handshake-secret phase."_el};
    }
    static constexpr auto cClientApplicationLabel = ByteArray<12U>{
        Byte{'c'},
        Byte{' '},
        Byte{'a'},
        Byte{'p'},
        Byte{' '},
        Byte{'t'},
        Byte{'r'},
        Byte{'a'},
        Byte{'f'},
        Byte{'f'},
        Byte{'i'},
        Byte{'c'}};
    static constexpr auto cServerApplicationLabel = ByteArray<12U>{
        Byte{'s'},
        Byte{' '},
        Byte{'a'},
        Byte{'p'},
        Byte{' '},
        Byte{'t'},
        Byte{'r'},
        Byte{'a'},
        Byte{'f'},
        Byte{'f'},
        Byte{'i'},
        Byte{'c'}};
    static constexpr auto cExporterMasterLabel = ByteArray<10U>{
        Byte{'e'}, Byte{'x'}, Byte{'p'}, Byte{' '}, Byte{'m'}, Byte{'a'}, Byte{'s'}, Byte{'t'}, Byte{'e'}, Byte{'r'}};

    auto clientApplicationSecret = ByteBlock{};
    auto serverApplicationSecret = ByteBlock{};
    auto exporterMasterSecret = ByteBlock{};
    const auto tlsHkdf = Tls13Hkdf{_algorithm};
    const auto clientEraseGuard = SecureEraseGuard{clientApplicationSecret};
    const auto serverEraseGuard = SecureEraseGuard{serverApplicationSecret};
    const auto exporterEraseGuard = SecureEraseGuard{exporterMasterSecret};

    _masterSecret.withUnprotectedData([&](const ConstByteSpan masterSecret) -> void {
        // RFC 8446 section 7.1: client_application_traffic_secret_0 binds the transcript through server Finished.
        clientApplicationSecret =
            tlsHkdf.deriveSecret(masterSecret, cClientApplicationLabel.span(), serverFinishedTranscriptHash);

        // RFC 8446 section 7.1: server_application_traffic_secret_0 uses the same transcript boundary.
        serverApplicationSecret =
            tlsHkdf.deriveSecret(masterSecret, cServerApplicationLabel.span(), serverFinishedTranscriptHash);

        // RFC 8446 sections 7.1 and 7.5: exporter_master_secret uses the server Finished transcript boundary.
        exporterMasterSecret =
            tlsHkdf.deriveSecret(masterSecret, cExporterMasterLabel.span(), serverFinishedTranscriptHash);
    });

    auto protectedClientApplication = ProtectedByteBlock{clientApplicationSecret.span()};
    auto protectedServerApplication = ProtectedByteBlock{serverApplicationSecret.span()};
    auto protectedExporterMaster = ProtectedByteBlock{exporterMasterSecret.span()};
    _clientApplicationSecret = std::move(protectedClientApplication);
    _serverApplicationSecret = std::move(protectedServerApplication);
    _exporterMasterSecret = std::move(protectedExporterMaster);
    _phase = Phase::Application;
}

auto Tls13KeySchedule::finishedVerifyData(const bool forClient, const ConstByteSpan transcriptHash) const -> ByteBlock {
    if (_phase != Phase::Handshake && _phase != Phase::Application) {
        throw LogicFailure{"TLS handshake traffic secrets are unavailable."_el};
    }
    return finishedFrom(forClient ? _clientHandshakeSecret : _serverHandshakeSecret, transcriptHash);
}

auto Tls13KeySchedule::verifyFinished(
    const bool forClient, const ConstByteSpan transcriptHash, const ConstByteSpan verifyData) const -> bool {
    // RFC 8446 section 4.4.4: compare the complete Hash.length verify_data without content-dependent short-circuiting.
    auto expected = finishedVerifyData(forClient, transcriptHash);
    const auto expectedEraseGuard = SecureEraseGuard{expected};
    return expected.isEqualConstTime(verifyData);
}

auto Tls13KeySchedule::handshakeTrafficSecret(const bool forClient) const -> TlsTrafficSecret {
    if (_phase != Phase::Handshake && _phase != Phase::Application) {
        throw LogicFailure{"TLS handshake traffic secrets are unavailable."_el};
    }
    return copyTrafficSecret(forClient ? _clientHandshakeSecret : _serverHandshakeSecret);
}

auto Tls13KeySchedule::applicationTrafficSecret(const bool forClient) const -> TlsTrafficSecret {
    if (_phase != Phase::Application && _phase != Phase::Finished) {
        throw LogicFailure{"TLS application traffic secrets are unavailable."_el};
    }
    return copyTrafficSecret(forClient ? _clientApplicationSecret : _serverApplicationSecret);
}

auto Tls13KeySchedule::exportKey(
    const ConstByteSpan label, const ConstByteSpan context, const unit::ByteCount outputLength) const -> ByteBlock {
    if (_phase != Phase::Application && _phase != Phase::Finished) {
        throw LogicFailure{"The TLS exporter master secret is unavailable."_el};
    }
    static constexpr auto cExporterLabel =
        ByteArray<8U>{Byte{'e'}, Byte{'x'}, Byte{'p'}, Byte{'o'}, Byte{'r'}, Byte{'t'}, Byte{'e'}, Byte{'r'}};

    // RFC 8446 section 7.5: context_hash = Hash(context_value), with empty context kept distinct from no context.
    auto contextHasher = Hasher{_algorithm};
    contextHasher.update(context);
    auto contextHash = contextHasher.finalize();
    const auto contextHashEraseGuard = SecureEraseGuard{contextHash};

    // RFC 8446 section 7.5: derived_secret = Derive-Secret(exporter_master_secret, Label, ""). Here "" means the
    // transcript hash of the empty message sequence, not a zero-length HkdfLabel context.
    const auto tlsHkdf = Tls13Hkdf{_algorithm};
    auto emptyTranscriptHash = tlsHkdf.emptyHash();
    auto derivedSecret = ByteBlock{};
    const auto emptyHashEraseGuard = SecureEraseGuard{emptyTranscriptHash};
    const auto derivedSecretEraseGuard = SecureEraseGuard{derivedSecret};
    _exporterMasterSecret.withUnprotectedData([&](const ConstByteSpan exporterMasterSecret) -> void {
        derivedSecret = tlsHkdf.deriveSecret(exporterMasterSecret, label, emptyTranscriptHash.span());
    });

    // RFC 8446 section 7.5: exported_keying_material = HKDF-Expand-Label(derived_secret, "exporter",
    // context_hash, key_length).
    return tlsHkdf.expandLabel(derivedSecret.span(), cExporterLabel.span(), contextHash.span(), outputLength);
}

void Tls13KeySchedule::discardResumptionMaster(const ConstByteSpan clientFinishedTranscriptHash) {
    if (_phase != Phase::Application) {
        throw LogicFailure{"TLS application secrets must exist before completing the key schedule."_el};
    }
    static constexpr auto cResumptionMasterLabel = ByteArray<10U>{
        Byte{'r'}, Byte{'e'}, Byte{'s'}, Byte{' '}, Byte{'m'}, Byte{'a'}, Byte{'s'}, Byte{'t'}, Byte{'e'}, Byte{'r'}};

    auto resumptionMasterSecret = ByteBlock{};
    const auto tlsHkdf = Tls13Hkdf{_algorithm};
    const auto resumptionEraseGuard = SecureEraseGuard{resumptionMasterSecret};
    _masterSecret.withUnprotectedData([&](const ConstByteSpan masterSecret) -> void {
        // RFC 8446 section 7.1: resumption_master_secret binds the transcript through client Finished. Tickets are
        // deferred, so the derived plaintext is intentionally erased without being retained or exposed.
        resumptionMasterSecret =
            tlsHkdf.deriveSecret(masterSecret, cResumptionMasterLabel.span(), clientFinishedTranscriptHash);
    });

    // The master secret no longer has a permitted consumer after the resumption derivation boundary.
    _masterSecret.secureErase();
    _clientHandshakeSecret.secureErase();
    _serverHandshakeSecret.secureErase();
    _phase = Phase::Finished;
}

void Tls13KeySchedule::secureErase() noexcept {
    // Keep terminal release order visible: earliest schedule stages first, then traffic and retained exporter state.
    _earlySecret.secureErase();
    _clientHandshakeSecret.secureErase();
    _serverHandshakeSecret.secureErase();
    _masterSecret.secureErase();
    _clientApplicationSecret.secureErase();
    _serverApplicationSecret.secureErase();
    _exporterMasterSecret.secureErase();
    _phase = Phase::Erased;
}

void Tls13KeySchedule::protectAndErase(ProtectedByteBlock &destination, ByteBlock &plaintext) {
    // Protect first so a provider failure leaves the caller's plaintext guard responsible for erasure.
    destination = ProtectedByteBlock{plaintext.span()};
    plaintext.secureErase();
}

auto Tls13KeySchedule::copyTrafficSecret(const ProtectedByteBlock &source) const -> TlsTrafficSecret {
    auto result = TlsTrafficSecret{};
    // Resolve only long enough for TlsTrafficSecret to create its own protected envelope.
    source.withUnprotectedData(
        [&](const ConstByteSpan secret) -> void { result = TlsTrafficSecret::fromBytes(_algorithm, secret); });
    return result;
}

auto Tls13KeySchedule::finishedFrom(const ProtectedByteBlock &baseSecret, const ConstByteSpan transcriptHash) const
    -> ByteBlock {
    static constexpr auto cFinishedLabel =
        ByteArray<8U>{Byte{'f'}, Byte{'i'}, Byte{'n'}, Byte{'i'}, Byte{'s'}, Byte{'h'}, Byte{'e'}, Byte{'d'}};
    auto result = ByteBlock{};
    const auto tlsHkdf = Tls13Hkdf{_algorithm};

    baseSecret.withUnprotectedData([&](const ConstByteSpan trafficSecret) -> void {
        // RFC 8446 section 4.4.4: finished_key = HKDF-Expand-Label(BaseKey, "finished", "", Hash.length).
        auto finishedKey = tlsHkdf.expandLabel(trafficSecret, cFinishedLabel.span(), {}, _algorithm.digestSize());
        finishedKey.markAsSensitive();

        // RFC 8446 section 4.4.4: verify_data = HMAC(finished_key, Transcript-Hash(Handshake Context, Certificate*,
        // CertificateVerify*)). Moving the key into Hmac makes its terminal erasure part of the keyed state lifetime.
        auto hmac = Hmac{_algorithm, std::move(finishedKey)};
        hmac.update(transcriptHash);
        result = hmac.finalize();
        result.markAsSensitive();
    });
    return result;
}

}
