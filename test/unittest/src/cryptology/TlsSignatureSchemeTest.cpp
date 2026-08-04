// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/tls/TlsSignatureScheme.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>

#include <cstdint>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(TlsSignatureScheme)
class TlsSignatureSchemeTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testMetadataAndRawValues() {
        REQUIRE_EQUAL(TlsSignatureScheme{}.toRawValue(), uint16_t{0x0804U});

        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPkcs1Sha256, 0x0401U, "rsa_pkcs1_sha256"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPkcs1Sha384, 0x0501U, "rsa_pkcs1_sha384"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::EcdsaSecp256r1Sha256, 0x0403U, "ecdsa_secp256r1_sha256"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::EcdsaSecp384r1Sha384, 0x0503U, "ecdsa_secp384r1_sha384"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPssRsaeSha256, 0x0804U, "rsa_pss_rsae_sha256"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPssRsaeSha384, 0x0805U, "rsa_pss_rsae_sha384"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::Ed25519, 0x0807U, "ed25519"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPssPssSha256, 0x0809U, "rsa_pss_pss_sha256"_el));
        WITH_CONTEXT(requireMetadata(TlsSignatureScheme::RsaPssPssSha384, 0x080aU, "rsa_pss_pss_sha384"_el));

        REQUIRE_FALSE(TlsSignatureScheme::fromRawValue(0x0806U).has_value());
        REQUIRE_FALSE(TlsSignatureScheme::fromRawValue(0xffffU).has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, TlsSignatureScheme::fromRawValueOrThrow(0xffffU));
    }

    void testUsagePolicy() {
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPkcs1Sha256, false, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPkcs1Sha384, false, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::EcdsaSecp256r1Sha256, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::EcdsaSecp384r1Sha384, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPssRsaeSha256, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPssRsaeSha384, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::Ed25519, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPssPssSha256, true, true));
        WITH_CONTEXT(requirePolicy(TlsSignatureScheme::RsaPssPssSha384, true, true));

        const auto invalid = TlsSignatureScheme{static_cast<TlsSignatureScheme::Value>(0xffffU)};
        REQUIRE_FALSE(invalid.isAllowedForCertificateVerify());
        REQUIRE_FALSE(invalid.isAllowedForCertificateSignature());
        REQUIRE(invalid.toString().isEmpty());
    }

private:
    void requireMetadata(const TlsSignatureScheme::Value value, const uint16_t rawValue, const el::String &name) {
        const auto scheme = TlsSignatureScheme{value};
        REQUIRE_EQUAL(scheme.toRawValue(), rawValue);
        REQUIRE_EQUAL(scheme.toString(), name);
        const auto parsed = TlsSignatureScheme::fromRawValue(rawValue);
        REQUIRE(parsed.has_value());
        REQUIRE_EQUAL(parsed->toRawValue(), rawValue);
        REQUIRE_EQUAL(TlsSignatureScheme::fromRawValueOrThrow(rawValue).toRawValue(), rawValue);
    }

    void requirePolicy(
        const TlsSignatureScheme::Value value, const bool certificateVerify, const bool certificateSignature) {
        const auto scheme = TlsSignatureScheme{value};
        REQUIRE_EQUAL(scheme.isAllowedForCertificateVerify(), certificateVerify);
        REQUIRE_EQUAL(scheme.isAllowedForCertificateSignature(), certificateSignature);
    }
};
