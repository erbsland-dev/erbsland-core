#include "MacosIdentityTestFixture.hpp"
#include "SystemIdentityStoreTestAccess.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/cryptology/impl/MacosSystemIdentityBackend.hpp>
#include <erbsland/cryptology/impl/NativeTlsAlgorithms.hpp>
#include <erbsland/unittest/UnitTest.hpp>

TESTED_TARGETS(MacosSystemIdentityBackend MacosTlsSigner SystemIdentityStore)
class MacosSystemIdentityNativeTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIsolatedKeychainSignatures() {
        using namespace el::cryptology;
        const auto applicationScope = ApplicationTestScope<>{};
        for (
            const auto fixtureName :
            {"data/network/tls-interop/native-ecdsa.p12.hex", "data/network/tls-interop/native-rsa.p12.hex"}) {
            const auto fixture = MacosIdentityTestFixture{fixtureName};
            auto store = SystemIdentityStore{};
            impl::SystemIdentityStoreTestAccess{store}.setBackend(
                std::make_shared<impl::MacosSystemIdentityBackend>(false, fixture.keychain()));
            const auto entries = store.list();
            REQUIRE_EQUAL(entries.count(), el::unit::ItemCount{1U});
            auto identity = store.load(entries.first().fingerprint);
            REQUIRE(identity.isNative());
            auto count = std::size_t{};
            for (const auto scheme : impl::native_tls_algorithms::schemes()) {
                if (!identity.selectSignatureScheme(el::util::List<TlsSignatureScheme>{scheme}).has_value()) {
                    continue;
                }
                const auto signature = identity.signTlsCertificateVerify(scheme, {});
                REQUIRE(entries.first().certificate.publicKey().verifyTlsCertificateVerifySignature(
                    scheme, {}, signature.span()));
                ++count;
            }
            REQUIRE(count > 0U);
            REQUIRE_THROWS_AS(
                CryptologyError, identity.signTlsCertificateVerify(TlsSignatureScheme::RsaPkcs1Sha256, {}));
        }
    }
};
