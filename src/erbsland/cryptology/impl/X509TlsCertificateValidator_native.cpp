#include "X509TlsCertificateValidator.hpp"

#include "algorithm/ecdsa_signature/EcdsaSignature.hpp"
#include "algorithm/ed25519_signature/Ed25519Signature.hpp"
#include "algorithm/rsa_signature/RsaSignature.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology::impl {

using namespace text::literals;

auto X509TlsCertificateValidator::validateNativePath(
    util::List<X509Certificate> path, const X509RevocationStatus revocationStatus) -> X509CertificateValidation {
    if (path.isEmpty() || _peerCertificates.isEmpty()) {
        return directRejection(
            X509CertificateValidationFailureCategory::EmptyPeerCertificates,
            {},
            "Native evaluation returned no certificate path."_el);
    }
    _path = std::move(path);
    if (_path.count().toSizeT() > X509TlsCertificatePolicy::cMaximumPathDepth ||
        _path.first().toDer() != _peerCertificates.certificates().first().toDer()) {
        return directRejection(
            X509CertificateValidationFailureCategory::NativeFailure,
            {},
            "Native evaluation returned an inconsistent or excessive certificate path."_el);
    }
    if (!validateReferenceIdentity()) {
        return rejectedResult();
    }
    if (!validateTarget(_path.first())) {
        return rejectedResult();
    }

    // RFC 8446 sections 4.2.3 and 4.4.2.2: native trust does not relax this library's modern key policy.
    // Keys and certificate signatures are public; the existing bounded verification arithmetic has no secrets.
    try {
        for (const auto &certificate : _path) {
            const auto key = certificate.publicKey();
            const auto oid = key.algorithm().oid().toString();
            if (oid == "1.2.840.113549.1.1.1"_el || oid == "1.2.840.113549.1.1.10"_el) {
                [[maybe_unused]] const auto validatedKey = rsa_signature::decodePublicKey(key);
            } else if (oid == "1.2.840.10045.2.1"_el) {
                [[maybe_unused]] const auto validatedKey =
                    ecdsa_signature::decodePublicKey(key, ecdsa_signature::decodeCurve(key));
            } else if (oid == "1.3.101.112"_el) {
                ed25519_signature::requireEd25519Algorithm(key.algorithm(), "public key"_el);
                const auto point = ed25519_signature::decodePoint(key.keyData().span());
                if (key.unusedBitCount() != 0U || !point.has_value() || !ed25519_signature::isPrimeOrderPoint(*point)) {
                    throw err::ParseError{"The native path contains an invalid Ed25519 key."_el};
                }
            } else {
                throw err::ParseError{"The native path contains an unsupported public-key algorithm."_el};
            }
        }
        for (auto index = std::size_t{}; index + 1U < _path.count().toSizeT(); ++index) {
            const auto child = _path.get(unit::ItemIndex::fromSizeT(index));
            const auto issuer = _path.get(unit::ItemIndex::fromSizeT(index + 1U));
            const auto status = verifyEdge(child, issuer);
            if (status != EdgeStatus::Valid) {
                return directRejection(
                    X509CertificateValidationFailureCategory::SignatureUnsupported,
                    child,
                    "The native path does not satisfy the modern certificate-signature policy."_el);
            }
        }
    } catch (const err::ParseError &error) {
        return directRejection(X509CertificateValidationFailureCategory::SignatureUnsupported, {}, error.reason());
    }

    // RFC 9525 section 6: native Common Name fallback never overrides the common SAN-only identity policy.
    if (!matchServerIdentity(_path.first())) {
        return directRejection(
            X509CertificateValidationFailureCategory::ServerIdentityMismatch,
            _path.first(),
            "No subjectAltName matches the requested DNS name or IP address."_el);
    }
    return X509CertificateValidation{_path, revocationStatus};
}

}
