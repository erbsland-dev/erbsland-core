// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <erbsland/Literals.hpp>
#include <erbsland/network/impl/tls/client/TlsClientProtocol.hpp>
#include <erbsland/network/impl/tls/server/TlsServerProtocol.hpp>
#include <erbsland/SigningPrivateKey.hpp>
#include <erbsland/TlsIdentity.hpp>
#include <erbsland/X509CertificateBundle.hpp>

#include <memory>

/// Shared deterministic identity and pending-operation driver for TLS fuzz targets.
namespace tls_fuzz_fixture {

/// Public test certificate and its software signing identity.
struct TestIdentity {
    erbsland::cryptology::X509CertificateBundle certificates;
    erbsland::cryptology::TlsIdentityConstPtr identity;
};

/// Load the fixed test identity once per fuzzing process.
[[nodiscard]] inline auto get() -> const TestIdentity & {
    using erbsland::text::String;
    using namespace erbsland::text::literals;
    static const auto result = []() -> TestIdentity {
        static constexpr auto cCertificate = R"PEM(-----BEGIN CERTIFICATE-----
MIIBOjCB7aADAgECAhQbMGnPk+uRDdUlVQ/WTX/UiQbF2jAFBgMrZXAwFDESMBAG
A1UEAwwJbG9jYWxob3N0MB4XDTI2MDgyMDA3MzExMVoXDTM2MDgxODA3MzExMVow
FDESMBAGA1UEAwwJbG9jYWxob3N0MCowBQYDK2VwAyEAFs6YtIaX6wy6BZ94JQi4
l6Rw+U23D90oV/1MpY7QGqejUTBPMAwGA1UdEwEB/wQCMAAwDgYDVR0PAQH/BAQD
AgeAMBMGA1UdJQQMMAoGCCsGAQUFBwMBMBoGA1UdEQQTMBGCCWxvY2FsaG9zdIcE
fwAAATAFBgMrZXADQQDCvxnKSOrrBNDvvXGjB18tu7U/zJHSbnJnD3b2OUu2ZB0M
/QuaBFG+UQA6kQMGJDlSDZOF4AcTsTuPKgnD4joC
-----END CERTIFICATE-----
)PEM"_el;
        static constexpr auto cPrivateKey = R"PEM(-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIGdkQSn1H8KGGlCkycDUBKxtm8xI+ri1WfZ5EUeHXPJD
-----END PRIVATE KEY-----
)PEM"_el;
        auto certificates = erbsland::cryptology::X509CertificateBundle::fromPemOrThrow(String{cCertificate});
        auto identity = std::make_shared<const erbsland::cryptology::TlsIdentity>(
            certificates, erbsland::cryptology::SigningPrivateKey::fromPemOrThrow(String{cPrivateKey}));
        return TestIdentity{std::move(certificates), std::move(identity)};
    }();
    return result;
}

/// Public client fixture used to reach mutual authentication with real validation and signatures.
[[nodiscard]] inline auto client() -> const TestIdentity & {
    using namespace erbsland::text::literals;
    static const auto result = []() -> TestIdentity {
        const auto certificates = erbsland::cryptology::X509CertificateBundle::fromPemOrThrow(
            erbsland::text::String{R"PEM(-----BEGIN CERTIFICATE-----
MIIBrzCCAVSgAwIBAgIBATAKBggqhkjOPQQDAjAjMSEwHwYDVQQDDBhFcmJzbGFu
ZC1tVExTLUludGVyb3AtQ0EwHhcNMjYxMDE4MTUxNTUwWhcNMzYxMDE1MTUxNTUw
WjAnMSUwIwYDVQQDDBxFcmJzbGFuZC1tVExTLUludGVyb3AtQ2xpZW50MFkwEwYH
KoZIzj0CAQYIKoZIzj0DAQcDQgAEmRFR83yWjT/sP/Ke1ZY1cl2pnZ3j35TANUGv
297i9kG3EUdnrXQPTmJLI8kergOJ4uRO3asGk+kLsComIeCTSKN1MHMwDAYDVR0T
AQH/BAIwADAOBgNVHQ8BAf8EBAMCB4AwEwYDVR0lBAwwCgYIKwYBBQUHAwIwHQYD
VR0OBBYEFBkE6ueebuGgsC6EnuIwr4Qegt5xMB8GA1UdIwQYMBaAFNHm7RzB6ePn
HL3jQFYxp6jvhljDMAoGCCqGSM49BAMCA0kAMEYCIQCqYSrN/bkBHv591HRaC2bt
F3B/4g2HlVVPi8CUzprGWAIhAMjHVr1TzFORGvayDbp+rwocRatlxS+DW/ORKK5L
fQTj
-----END CERTIFICATE-----
)PEM"_el});
        auto identity = std::make_shared<const erbsland::cryptology::TlsIdentity>(
            certificates,
            erbsland::cryptology::SigningPrivateKey::fromPemOrThrow(
                erbsland::text::String{R"PEM(-----BEGIN PRIVATE KEY-----
MIGHAgEAMBMGByqGSM49AgEGCCqGSM49AwEHBG0wawIBAQQgRq1M1B87M6tELaAO
05JiJIyaAfScq6K5VZ2/Cij8ncmhRANCAASZEVHzfJaNP+w/8p7VljVyXamdnePf
lMA1Qa/b3uL2QbcRR2etdA9OYksjyR6uA4ni5E7dqwaT6QuwKiYh4JNI
-----END PRIVATE KEY-----
)PEM"_el}));
        return TestIdentity{certificates, std::move(identity)};
    }();
    return result;
}

/// Execute client validation and signing requests before advancing checkpoints.
inline void resume(erbsland::network::impl::TlsClientProtocol &protocol) {
    while (protocol.hasCheckpoint()) {
        if (const auto &validation = protocol.pendingCertificateValidation()) {
            protocol.completeCertificateValidation(validation->execute());
        } else if (const auto &signature = protocol.pendingSignature()) {
            protocol.completeSignature(signature->execute());
        } else {
            protocol.resume();
        }
    }
}

/// Execute server trust and signing requests before advancing checkpoints.
inline void resume(erbsland::network::impl::TlsServerProtocol &protocol) {
    while (protocol.hasCheckpoint()) {
        if (const auto &validation = protocol.pendingCertificateValidation()) {
            protocol.completeCertificateValidation(validation->execute());
        } else if (const auto &signature = protocol.pendingSignature()) {
            protocol.completeSignature(signature->execute());
        } else {
            protocol.resume();
        }
    }
}

}
