#include "NativeTlsAlgorithms.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"

#include "algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "algorithm/rsa_signature/RsaSignature.hpp"

#include "../CryptologyError.hpp"
#include "../Hasher.hpp"
#include "../x509/X509Certificate.hpp"

#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl::native_tls_algorithms {

using namespace mem;
using namespace text::literals;

auto schemes() -> util::List<TlsSignatureScheme> {
    return util::List<TlsSignatureScheme>{
        TlsSignatureScheme::EcdsaSecp256r1Sha256,
        TlsSignatureScheme::EcdsaSecp384r1Sha384,
        TlsSignatureScheme::RsaPssRsaeSha256,
        TlsSignatureScheme::RsaPssRsaeSha384,
        TlsSignatureScheme::RsaPssRsaeSha512,
        TlsSignatureScheme::RsaPssPssSha256,
        TlsSignatureScheme::RsaPssPssSha384,
        TlsSignatureScheme::RsaPssPssSha512};
}

auto matches(const PublicKey &key, const TlsSignatureScheme scheme) noexcept -> bool {
    try {
        // RFC 8446 section 4.2.3: TLS ECDSA schemes bind both curve and hash, unlike certificate signatures.
        if (scheme == TlsSignatureScheme::EcdsaSecp256r1Sha256 || scheme == TlsSignatureScheme::EcdsaSecp384r1Sha384) {
            const auto curve = ecdsa_signature::decodeCurve(key);
            return (curve == NistPrimeCurve::Name::P256 && scheme == TlsSignatureScheme::EcdsaSecp256r1Sha256) ||
                (curve == NistPrimeCurve::Name::P384 && scheme == TlsSignatureScheme::EcdsaSecp384r1Sha384);
        }
        const auto raw = scheme.toRawValue();
        const auto oid = key.algorithm().oid().toString();
        if (!((raw >= 0x0804U && raw <= 0x0806U && oid == cryptology_oids::rsaEncryption) ||
                (raw >= 0x0809U && raw <= 0x080bU && oid == cryptology_oids::rsaPss))) {
            return false;
        }
        // RFC 4055 section 3: preserve SPKI restrictions; native capability alone cannot widen key permissions.
        const auto decoded = rsa_signature::decodePublicKey(key);
        const auto parameters = rsa_signature::decodeSignatureParameters(scheme.signatureAlgorithmIdentifier());
        return rsa_signature::parametersMatchKey(parameters, decoded.restrictions);
    } catch (const err::RuntimeError &) {
        return false;
    }
}

auto hash(const TlsSignatureScheme scheme) -> HashAlgorithm {
    switch (scheme.toRawValue()) {
    case 0x0403U:
    case 0x0804U:
    case 0x0809U:
        return HashAlgorithm::Sha2_256;
    case 0x0503U:
    case 0x0805U:
    case 0x080aU:
        return HashAlgorithm::Sha2_384;
    case 0x0806U:
    case 0x080bU:
        return HashAlgorithm::Sha2_512;
    default:
        throw CryptologyError{"Unsupported native TLS signing scheme."_el};
    }
}

auto fingerprint(const ByteBlock &der) -> ByteBlock {
    auto hasher = Hasher{HashAlgorithm::Sha2_256};
    hasher.update(der);
    return hasher.finalize();
}

auto isSelfSigned(const X509Certificate &certificate) noexcept -> bool {
    // RFC 5280 section 3.2: matching names establish self-issuance, not a signature by this public key.
    if (certificate.subjectId() != certificate.issuerId()) {
        return false;
    }
    try {
        return certificate.publicKey().verifySignature(
            certificate.signatureAlgorithm(),
            certificate.tbsCertificateDer().span(),
            certificate.signatureData().span());
    } catch (const err::RuntimeError &) {
        return false;
    }
}

auto ecdsaDer(const ConstByteSpan signature) -> ByteBlock {
    if (signature.size() != 64U && signature.size() != 96U) {
        throw CryptologyError{"A native provider returned an invalid ECDSA signature width."_el};
    }
    // RFC 3279 section 2.2.3: native r || s becomes a canonical sequence of positive DER integers.
    auto encoder = DerEncoder{};
    const auto sequence = encoder.beginSequence();
    encoder.appendPositiveInteger(signature.first(signature.size() / 2U));
    encoder.appendPositiveInteger(signature.subspan(signature.size() / 2U));
    encoder.end(sequence);
    return encoder.encoded();
}

}
