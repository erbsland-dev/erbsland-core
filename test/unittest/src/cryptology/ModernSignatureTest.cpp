#include "CryptologyResponseKeys.hpp"
#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/Literals.hpp>
#include <erbsland/PublicKey.hpp>
#include <erbsland/TlsSignatureScheme.hpp>
#include <erbsland/X509AlgorithmIdentifier.hpp>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(EcdsaSignature RsaSignature TlsSignatureScheme)
class ModernSignatureTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testIndependentSha2Signatures() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/modern_signatures.txt"_el}.read();
        REQUIRE_EQUAL(records.size(), std::size_t{8U});
        for (const auto &record : records) {
            const auto key = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
            const auto algorithm =
                X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(record.setting("AlgorithmDer"_el)));
            const auto message = bytesFromHex(record.value(response_key::cMessageField));
            const auto signature = bytesFromHex(record.value(response_key::cSignatureField));
            REQUIRE(key.verifySignature(algorithm, message.span(), signature.span()));
            REQUIRE_FALSE(key.verifySignature(algorithm, {}, signature.span()));
        }
    }

    void testPssAbsentAndNullHashParameters() {
        const auto records = CryptologyResponseReader{"data/cryptology/signature/PSS_SHA256.rsp"_el}.read();
        const auto &record = records.front();
        const auto key = PublicKey::fromDerOrThrow(bytesFromHex(record.setting("PublicKeyDer"_el)));
        const auto message = bytesFromHex(record.value(response_key::cMessageField));
        const auto signature = bytesFromHex(record.value(response_key::cSignatureField));
        const auto absent = X509AlgorithmIdentifier::fromDerOrThrow(bytesFromHex(
            "303d06092a864886f70d01010a3030a00d300b0609608648016503040201"
            "a11a301806092a864886f70d010108300b0609608648016503040201a203020120"));
        REQUIRE(key.verifySignature(absent, message.span(), signature.span()));
        REQUIRE(key.verifySignature(
            TlsSignatureScheme{TlsSignatureScheme::RsaPssRsaeSha256}.signatureAlgorithmIdentifier(),
            message.span(),
            signature.span()));
    }
};
