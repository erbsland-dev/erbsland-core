// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EcdsaSignature.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../../unit/ItemCount.hpp"
#include "../../../../unit/ItemIndex.hpp"
#include "../../../Hasher.hpp"
#include "../../DerParser.hpp"

namespace erbsland::cryptology::impl::ecdsa_signature {

using namespace text::literals;
using namespace unit;

auto isSignatureAlgorithm(const X509AlgorithmIdentifier &algorithm) noexcept -> bool {
    const auto oid = algorithm.oid().toString();
    return oid == "1.2.840.10045.4.3.2"_el || oid == "1.2.840.10045.4.3.3"_el;
}

auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    const mem::ConstByteSpan message,
    const mem::ConstByteSpan signature) -> bool {
    const auto parameters = decodeParameters(signatureAlgorithm);
    const auto curve = NistPrimeCurve{parameters.curve};
    const auto publicPoint = decodePublicKey(publicKey, parameters);
    const auto values = decodeSignature(signature, curve);

    // FIPS 186-5 section 6.4.2 step 1: reject r or s outside [1,n-1].
    if (!values.has_value()) {
        return false;
    }

    // FIPS 186-5 section 6.4.2 steps 2--3: hash M and interpret the leftmost min(bitlen(n), hashlen) bits as e.
    // The selected pairings have equal order and digest widths, so the complete SHA-256 or SHA-384 digest is used.
    auto hasher = Hasher{parameters.hash};
    hasher.update(message);
    const auto digest = hasher.finalize();
    const auto e = curve.numberFromBigEndian(digest.span());
    if (!e.has_value()) {
        throwParseError("The ECDSA message digest exceeds the selected scalar width."_el);
    }

    // FIPS 186-5 section 6.4.2 steps 4--5: w=s^-1 mod n, u=e*w mod n, and v=r*w mod n.
    const auto w = curve.invertOrder(values->s);
    const auto u = curve.multiplyOrder(*e, w);
    const auto v = curve.multiplyOrder(values->r, w);

    // FIPS 186-5 section 6.4.2 step 6: R1=[u]G+[v]Q; the identity is an invalid signature.
    const auto r1 = curve.addPublic(curve.multiplyPublic(curve.basePoint(), u), curve.multiplyPublic(publicPoint, v));
    const auto affine = curve.toAffine(r1);
    if (!affine.has_value()) {
        return false;
    }

    // FIPS 186-5 section 6.4.2 steps 7--9: convert xR to an integer and compare r with xR mod n.
    return curve.compare(values->r, curve.reduceOrder(affine->x)) == 0;
}

auto decodeParameters(const X509AlgorithmIdentifier &algorithm) -> Parameters {
    // RFC 5758 section 3.2 identifies ECDSA/SHA-2 by distinct OIDs and requires AlgorithmIdentifier parameters absent.
    if (!algorithm.parameters().isEmpty()) {
        throwParseError("ECDSA signature AlgorithmIdentifier parameters must be absent."_el);
    }
    const auto oid = algorithm.oid().toString();
    if (oid == "1.2.840.10045.4.3.2"_el) {
        return {NistPrimeCurve::Name::P256, HashAlgorithm::Sha2_256};
    }
    if (oid == "1.2.840.10045.4.3.3"_el) {
        return {NistPrimeCurve::Name::P384, HashAlgorithm::Sha2_384};
    }
    throwParseError("The X.509 signature algorithm is not a supported ECDSA SHA-2 algorithm."_el);
}

auto decodePublicKey(const PublicKey &publicKey, const Parameters &parameters) -> NistPrimeCurve::Point {
    const auto &keyAlgorithm = publicKey.algorithm();

    // RFC 5480 section 2.1 requires id-ecPublicKey for an unrestricted ECDSA verification key.
    if (keyAlgorithm.oid().toString() != "1.2.840.10045.2.1"_el) {
        throwParseError("SubjectPublicKeyInfo does not contain an id-ecPublicKey key."_el);
    }

    // RFC 5480 section 2.1.1 requires a present namedCurve OID and forbids implicit or explicitly specified curves.
    const auto curveOid = keyAlgorithm.parameters().toObjectIdentifier();
    if (!curveOid.has_value()) {
        throwParseError("An ECDSA public key requires named-curve OBJECT IDENTIFIER parameters."_el);
    }
    auto keyCurve = NistPrimeCurve::Name::P256;
    if (curveOid->toString() == "1.2.840.10045.3.1.7"_el) {
        keyCurve = NistPrimeCurve::Name::P256;
    } else if (curveOid->toString() == "1.3.132.0.34"_el) {
        keyCurve = NistPrimeCurve::Name::P384;
    } else {
        throwParseError("SubjectPublicKeyInfo uses an unsupported named curve."_el);
    }
    if (keyCurve != parameters.curve) {
        throwParseError("The ECDSA signature hash and public-key curve are not a supported matched pair."_el);
    }
    if (publicKey.unusedBitCount() != 0U) {
        throwParseError("An ECDSA public-key BIT STRING must not contain unused bits."_el);
    }

    // RFC 5480 section 2.2 and SP 800-186 appendix D.1 require a supported encoding and full public-point validation.
    const auto curve = NistPrimeCurve{keyCurve};
    const auto point = curve.pointFromBytes(publicKey.keyData().span());
    if (!point.has_value()) {
        throwParseError("The ECDSA public point is malformed or is not on the selected curve."_el);
    }
    return *point;
}

auto decodeSignature(const mem::ConstByteSpan signature, const NistPrimeCurve &curve)
    -> std::optional<SignatureValues> {
    // RFC 3279 section 2.2.3 bounds P-256/P-384 ECDSA-Sig-Value to 72/104 DER octets with canonical INTEGERs.
    const auto maximumLength = curve.name() == NistPrimeCurve::Name::P256 ? std::size_t{72U} : std::size_t{104U};
    if (signature.size() > maximumLength) {
        throwParseError("The ECDSA signature exceeds the fixed curve-specific DER bound."_el);
    }

    // RFC 3279 section 2.2.3: ECDSA-Sig-Value is exactly SEQUENCE { r INTEGER, s INTEGER } in canonical DER.
    const auto root = DerParser{mem::ByteBlock::fromSpan(signature)}.parseDocument();
    requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "ECDSA-Sig-Value"_el);
    if (root.childCount() != ItemCount{2U}) {
        throwParseError("ECDSA-Sig-Value must contain exactly r and s."_el);
    }
    const auto r = decodeInteger(root.child(ItemIndex{0U}), curve);
    const auto s = decodeInteger(root.child(ItemIndex{1U}), curve);
    if (!r.has_value() || !s.has_value()) {
        return std::nullopt;
    }
    return SignatureValues{*r, *s};
}

auto decodeInteger(const Asn1Node &node, const NistPrimeCurve &curve) -> std::optional<NistPrimeCurve::Number> {
    requireNode(
        node,
        Asn1TagClass::Universal,
        static_cast<uint32_t>(Asn1UniversalType::Integer),
        false,
        "ECDSA signature INTEGER"_el);
    auto contents = node.contentData();
    if (contents.isEmpty() || (contents.span().front().toUInt8() & 0x80U) != 0U) {
        throwParseError("An ECDSA signature INTEGER must be nonnegative."_el);
    }
    if (contents.span().front().toUInt8() == 0U && contents.length() > ByteLength::one()) {
        contents = contents.slice(ByteIndex{1U}, contents.length() - ByteLength::one());
    }
    const auto value = curve.numberFromBigEndian(contents.span());
    if (!value.has_value()) {
        return std::nullopt;
    }
    if (curve.isZero(*value) || curve.compare(*value, curve.order()) >= 0) {
        return std::nullopt;
    }
    return value;
}

void requireNode(
    const Asn1Node &node,
    const Asn1TagClass tagClass,
    const uint32_t tagNumber,
    const bool constructed,
    [[maybe_unused]] const text::String &name) {
    if (node.isEmpty() || node.tagClass() != tagClass || node.tagNumber() != tagNumber ||
        node.isConstructed() != constructed) {
        throwParseError("An ECDSA ASN.1 value has an unexpected type or form."_el);
    }
}

void throwParseError(const text::String &reason) {
    throw err::ParseError{reason};
}

}
