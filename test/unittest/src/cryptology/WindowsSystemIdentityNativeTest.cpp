#include "CryptologyTestHelper.hpp"
#include "SystemIdentityStoreTestAccess.hpp"

#include <erbsland/cryptology/impl/NativeTlsAlgorithms.hpp>
#include <erbsland/cryptology/impl/WindowsSystemIdentityBackend.hpp>
#include <erbsland/CryptologyError.hpp>

TESTED_TARGETS(WindowsSystemIdentityBackend WindowsTlsSigner SystemIdentityStore)
class WindowsSystemIdentityNativeTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIsolatedMemoryStoreSignatures() {
        using namespace el::cryptology;
        for (
            const auto fixtureName :
            {"data/network/tls-interop/native-ecdsa.p12.hex", "data/network/tls-interop/native-rsa.p12.hex"}) {
            auto data = bytesFromHex(el::text::String{el::unittest::fh::readDataText(fixtureName)}.trimmed());
            data.markAsSensitive();
            auto native = CRYPT_DATA_BLOB{};
            native.cbData = static_cast<DWORD>(data.length().toSizeT());
            native.pbData = const_cast<BYTE *>(reinterpret_cast<const BYTE *>(data.span().data()));
            auto resources = impl::WindowsIdentityResources{};
            resources.store = PFXImportCertStore(
                &native, L"fixture-only", CRYPT_USER_KEYSET | PKCS12_NO_PERSIST_KEY | PKCS12_ALWAYS_CNG_KSP);
            data.secureErase();
            REQUIRE(resources.store != nullptr);
            auto store = SystemIdentityStore{};
            impl::SystemIdentityStoreTestAccess{store}.setBackend(
                std::make_shared<impl::WindowsSystemIdentityBackend>(false, resources.store));
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
