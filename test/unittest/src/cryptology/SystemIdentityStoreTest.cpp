#include "CryptologyTestHelper.hpp"
#include "MockNativeTlsSigner.hpp"
#include "MockSystemIdentityBackend.hpp"
#include "SystemIdentityStoreTestAccess.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/impl/NativeTlsAlgorithms.hpp>
#include <erbsland/CryptologyError.hpp>
#include <erbsland/Definitions.hpp>
#include <erbsland/ParameterFailure.hpp>
#include <erbsland/SystemIdentityStore.hpp>

using namespace el::cryptology;

TESTED_TARGETS(SystemIdentityStore SystemIdentityInfo NativeTlsAlgorithms)
class SystemIdentityStoreTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testOpaqueSignerLifetimeSchemesAndFaultDetection() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto chain = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/server.pem")});
        auto signer = std::make_shared<MockNativeTlsSigner>(SigningPrivateKey::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/server-key.pem")}));
        auto backend = std::make_shared<MockSystemIdentityBackend>(chain, signer);
        auto store = SystemIdentityStore{};
        impl::SystemIdentityStoreTestAccess{store}.setBackend(backend);
        auto identity = store.load(store.list().first().fingerprint);
        store = SystemIdentityStore{};
        backend.reset();
        REQUIRE(identity.isNative());
        REQUIRE_FALSE(
            identity.selectSignatureScheme(el::util::List<TlsSignatureScheme>{TlsSignatureScheme::RsaPssRsaeSha256})
                .has_value());
        const auto signature = identity.signTlsCertificateVerify(TlsSignatureScheme::RsaPssRsaeSha512, {});
        REQUIRE_FALSE(signature.isEmpty());
        signer->fail = true;
        REQUIRE_THROWS_AS(CryptologyError, identity.signTlsCertificateVerify(TlsSignatureScheme::RsaPssRsaeSha512, {}));
    }

    void testPlatformCapabilityAndInvalidFingerprint() {
        const auto store = SystemIdentityStore{};
#if defined(ERBSLAND_OS_LINUX)
        REQUIRE_FALSE(store.isSupported());
        REQUIRE_THROWS_AS(CryptologyError, store.list());
#else
        REQUIRE(store.isSupported());
#endif
        REQUIRE_THROWS_AS(el::err::ParameterFailure, store.load({}));
    }

    void testEcdsaNativeSignatureNormalization() {
        auto native = el::mem::ByteBlockEditor{el::unit::ByteCount{64U}};
        native.setOrThrow(el::unit::ByteIndex{31U}, el::mem::Byte{1U});
        native.setOrThrow(el::unit::ByteIndex{63U}, el::mem::Byte{2U});
        REQUIRE_EQUAL(impl::native_tls_algorithms::ecdsaDer(native.span()), bytesFromHex("3006020101020102"));
        REQUIRE_THROWS_AS(CryptologyError, impl::native_tls_algorithms::ecdsaDer({}));
    }

    void testCertificateFingerprintIsSha256() {
        REQUIRE_EQUAL(
            impl::native_tls_algorithms::fingerprint({}),
            bytesFromHex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSelfIssuedCertificateRequiresValidSelfSignature() {
        const auto certificate = X509CertificateBundle::fromPemOrThrow(
            el::text::String{el::unittest::fh::readDataText("data/network/tls-interop/server-ecdsa.pem")})
                                     .certificates()
                                     .first();
        REQUIRE(impl::native_tls_algorithms::isSelfSigned(certificate));
        auto modified = el::mem::ByteBlockEditor{certificate.toDer()};
        modified.xorAt(modified.lastIndex(), el::mem::Byte{1U});
        const auto selfIssued =
            X509Certificate::fromDerOrThrow(modified.slice(el::unit::ByteIndex::zero(), modified.length()));
        REQUIRE_EQUAL(selfIssued.subjectId(), selfIssued.issuerId());
        REQUIRE_FALSE(impl::native_tls_algorithms::isSelfSigned(selfIssued));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMissingNativeIdentityFailsWithoutInteraction() {
        const auto store = SystemIdentityStore{};
        REQUIRE_THROWS_AS(CryptologyError, store.load(el::mem::ByteBlock{el::unit::ByteCount{32U}}));
    }
};
