// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "EcSigningKeyTestApplication.hpp"
#include "EcSigningKeyTestHelper.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/impl/EcPrivateScalarPolicy.hpp>
#include <erbsland/cryptology/impl/EncryptedPrivateKeyCodec.hpp>
#include <erbsland/cryptology/impl/PemCodec.hpp>
#include <erbsland/cryptology/impl/PemLabel.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/ParseError.hpp>
#include <erbsland/RandomFailure.hpp>
#include <erbsland/SecureRandom.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/TlsSignatureScheme.hpp>

#include <type_traits>
#include <vector>

using namespace el::cryptology;
using namespace el::cryptology::impl;
using namespace el::text::literals;

TESTED_TARGETS(EcPrivateScalarPolicy SigningKeyGenerator SigningPrivateKey PrivateKeyParser)
class EcSigningKeyTest final : public UNITTEST_SUBCLASS(EcSigningKeyTestHelper) {
public:
    void testGenerationRetryLight() { WITH_CONTEXT(requireGeneration(CurveName::P256, 3U)); }

    void testWeakImportLight() {
        for (const auto name : cCurves) {
            WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, scalarBytes(name))));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testFirstCandidateAccepted() {
        for (const auto name : cCurves) {
            WITH_CONTEXT(requireGeneration(name, 0U));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSuspiciousCandidatesAreDiscarded() {
        for (const auto name : cCurves) {
            WITH_CONTEXT(requireGeneration(name, 3U));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testLastCandidateAccepted() {
        for (const auto name : cCurves) {
            WITH_CONTEXT(requireGeneration(name, 127U));
        }
    }

    void testExhaustionIsFatalAndBounded() {
        static_assert(std::is_base_of_v<el::err::LogicFailure, el::random::RandomFailure>);
        for (const auto name : cCurves) {
            auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
            auto &random = scope.app().scriptedRandom();
            // Repeat a tiny, nonzero scalar so a removed policy would immediately return an insecure key.
            random.arm({scalarBytes(name)}, true);
            auto failed = false;
            try {
                (void)SigningPrivateKey::generate(profile(name));
            } catch (const el::random::RandomFailure &error) {
                failed = true;
                REQUIRE_EQUAL(
                    error.reason(),
                    el::StringFormat{"EC P-{} signing-key generation rejected all 128 random candidates."_el}.build(
                        NistPrimeCurve{name}.byteLength().toSizeT() * 8U));
            }
            REQUIRE(failed);
            REQUIRE_EQUAL(random.scriptedDraws(), 128U);
            REQUIRE_EQUAL(random.otherDraws(), 0U);
        }
    }

    void testInvalidRangeExhaustionIsFatalAndBounded() {
        for (const auto name : cCurves) {
            for (const auto fill : {uint8_t{0U}, uint8_t{255U}}) {
                auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
                auto &random = scope.app().scriptedRandom();
                auto bytes = el::ByteBlockEditor{scalarBytes(name)};
                bytes.fill(el::Byte{fill});
                random.arm({bytes}, true);
                REQUIRE_THROWS_AS(el::random::RandomFailure, SigningPrivateKey::generate(profile(name)));
                REQUIRE_EQUAL(random.scriptedDraws(), 128U);
                REQUIRE_EQUAL(random.otherDraws(), 0U);
            }
        }
    }

    void testEntropyFailureIsNotRetried() {
        for (const auto name : cCurves) {
            auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
            auto &random = scope.app().scriptedRandom();
            random.armFailure();
            auto failed = false;
            try {
                (void)SigningPrivateKey::generate(profile(name));
            } catch (const el::random::RandomFailure &error) {
                failed = true;
                REQUIRE_EQUAL(error.reason(), "Scripted entropy source failure"_el);
            }
            REQUIRE(failed);
            REQUIRE_EQUAL(random.scriptedDraws(), 1U);
            REQUIRE_EQUAL(random.otherDraws(), 0U);
        }
    }

    void testScopedEntropyIsRestored() {
        {
            auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
            scope.app().scriptedRandom().armFailure();
            REQUIRE_THROWS_AS(el::random::RandomFailure, SigningPrivateKey::generate(profile(CurveName::P256)));
        }
        const auto scope = ApplicationTestScope<>{};
        REQUIRE(dynamic_cast<el::random::SecureRandom *>(&el::core::application().secureRandom()) != nullptr);
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testWeakImportsAreRecoverable() {
        for (const auto name : cCurves) {
            auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
            // An invalid import must fail before any protected-storage entropy is requested.
            auto &random = scope.app().scriptedRandom();
            random.armFailure();
            const auto curve = NistPrimeCurve{name};
            auto nearOrder = curve.order();
            --nearOrder.words()[0U];
            for (const auto &bytes : {scalarBytes(name), curve.numberToBigEndian(nearOrder)}) {
                WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, bytes)));
                WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, bytes, true)));
            }
            auto allOnes = el::ByteBlockEditor{scalarBytes(name)};
            allOnes.fill(el::Byte{255U});
            for (
                const auto &bytes :
                {scalarBytes(name, 0U),
                    curve.numberToBigEndian(curve.order()),
                    el::ByteBlock{allOnes},
                    el::ByteBlock{}}) {
                WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, bytes)));
            }
            const auto valid = acceptedScalar(name);
            WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, el::ByteBlock::fromSpan(valid.span().subspan(1U)))));
            auto oversized = el::ByteBlockEditor{};
            oversized.append(el::Byte{}).append(valid);
            WITH_CONTEXT(requireRejectedImport(privateKeyDer(name, oversized)));
            REQUIRE_EQUAL(random.scriptedDraws(), 0U);
            REQUIRE_EQUAL(random.otherDraws(), 0U);
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAcceptedImportRoundTripsAndSigns() {
        for (const auto name : cCurves) {
            const auto scope = ApplicationTestScope<>{};
            el::core::application().cryptologyConfiguration().setProtectedDataMode(ProtectedDataMode::InternalOnly);
            const auto scalar = acceptedScalar(name);
            const auto der = privateKeyDer(name, scalar, true);
            const auto pem = PemCodec{der, PemLabel::PrivateKey}.encode();
            const auto key = SigningPrivateKey::fromDerOrThrow(der);
            REQUIRE_EQUAL(exportedScalar(key), scalar);
            REQUIRE(SigningPrivateKey::fromDer(der).matches(key.publicKey()));
            REQUIRE(SigningPrivateKey::fromPem(pem).matches(key.publicKey()));
            REQUIRE(SigningPrivateKey::fromPemOrThrow(pem).matches(key.publicKey()));
            auto scheme = TlsSignatureScheme{TlsSignatureScheme::EcdsaSecp256r1Sha256};
            if (name == CurveName::P384) {
                scheme = TlsSignatureScheme::EcdsaSecp384r1Sha384;
            }
            const auto message = bytesFromHex("01020304");
            const auto signature = key.signTlsCertificateVerify(scheme, message.span());
            REQUIRE(key.publicKey().verifyTlsCertificateVerifySignature(scheme, message.span(), signature.span()));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testEncryptedWeakImportRetainsGenericFailure() {
        for (const auto name : cCurves) {
            const auto scope = ApplicationTestScope<>{};
            const auto der = privateKeyDer(name, scalarBytes(name));
            // Empty passwords remain supported for import; encryption here uses the internal fixture codec.
            const auto encrypted = encrypted_private_key_codec::encrypt(der.span(), {});
            const auto pem = PemCodec{encrypted, PemLabel::EncryptedPrivateKey}.encode();
            auto failed = false;
            try {
                (void)SigningPrivateKey::fromEncryptedDerOrThrow(encrypted, {});
            } catch (const CryptologyError &error) {
                failed = true;
                REQUIRE_EQUAL(error.reason(), "Private-key decryption failed."_el);
            }
            REQUIRE(failed);
            REQUIRE(SigningPrivateKey::fromEncryptedDer(encrypted, {}).isEmpty());
            failed = false;
            try {
                (void)SigningPrivateKey::fromEncryptedPemOrThrow(pem, {});
            } catch (const CryptologyError &error) {
                failed = true;
                REQUIRE_EQUAL(error.reason(), "Private-key decryption failed."_el);
            }
            REQUIRE(failed);
            REQUIRE(SigningPrivateKey::fromEncryptedPem(pem, {}).isEmpty());
        }
    }

private:
    void requireGeneration(const CurveName name, const std::size_t rejectedCount) {
        auto scope = ApplicationTestScope<el::test::EcSigningKeyTestApplication>{};
        auto &random = scope.app().scriptedRandom();
        const auto scalar = acceptedScalar(name);
        auto candidates = std::vector<el::ByteBlock>(rejectedCount, scalarBytes(name));
        if (rejectedCount > 1U) {
            candidates[0U] = scalarBytes(name, 0U);
            auto nearOrder = NistPrimeCurve{name}.order();
            --nearOrder.words()[0U];
            candidates[1U] = NistPrimeCurve{name}.numberToBigEndian(nearOrder);
        }
        candidates.push_back(scalar);
        random.arm(std::move(candidates));
        const auto key = SigningPrivateKey::generate(profile(name));
        REQUIRE_EQUAL(exportedScalar(key), scalar);
        REQUIRE_EQUAL(random.scriptedDraws(), rejectedCount + 1U);
        REQUIRE_GREATER(random.otherDraws(), 0U);
        REQUIRE(SigningPrivateKey::fromDerOrThrow(key.toDer()).matches(key.publicKey()));
    }

    void requireRejectedImport(const el::ByteBlock &der) {
        const auto pem = PemCodec{der, PemLabel::PrivateKey}.encode();
        REQUIRE_THROWS_AS(el::err::ParseError, SigningPrivateKey::fromDerOrThrow(der));
        REQUIRE_THROWS_AS(el::err::ParseError, SigningPrivateKey::fromPemOrThrow(pem));
        REQUIRE(SigningPrivateKey::fromDer(der).isEmpty());
        REQUIRE(SigningPrivateKey::fromPem(pem).isEmpty());
    }
};
