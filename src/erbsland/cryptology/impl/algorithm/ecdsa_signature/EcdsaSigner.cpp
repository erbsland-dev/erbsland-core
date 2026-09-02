// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EcdsaSigner.hpp"

#include "../../../../err/ParameterError.hpp"
#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../CryptologyError.hpp"
#include "../../../HashAlgorithm.hpp"
#include "../../../Hasher.hpp"
#include "../../../Hmac.hpp"
#include "../../DerEncoder.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::ecdsa_signer {

using namespace text::literals;

auto publicKey(const mem::ConstByteSpan privateScalar, const NistPrimeCurve::Name curveName) -> mem::ByteBlock {
    const auto curve = NistPrimeCurve{curveName};
    auto scalar = decodePrivateScalar(privateScalar, curveName);
    const auto scalarEraseGuard = SecureEraseGuard{scalar};

    // SEC 1 section 3.2.1: Q = dG. The secret path processes every one of the 256 scalar bits.
    auto point = curve.multiplyBaseSecret(scalar);
    const auto pointEraseGuard = SecureEraseGuard{point};
    auto affinePoint = curve.toAffineSecret(point);
    const auto affinePointEraseGuard = SecureEraseGuard{affinePoint};
    return curve.pointToUncompressed(affinePoint);
}

auto sign(
    const mem::ConstByteSpan privateScalar, const mem::ConstByteSpan message, const NistPrimeCurve::Name curveName)
    -> mem::ByteBlock {
    const auto curve = NistPrimeCurve{curveName};
    const auto hashAlgorithm =
        curveName == NistPrimeCurve::Name::P256 ? HashAlgorithm::Sha2_256 : HashAlgorithm::Sha2_384;
    auto d = decodePrivateScalar(privateScalar, curveName);
    const auto dEraseGuard = SecureEraseGuard{d};

    // FIPS 186-5 section 6.4.1 step 1: e = Hash(M), using SHA-256 for P-256.
    auto digest = hashMessage(message, hashAlgorithm);
    digest.markAsSensitive();
    const auto digestEraseGuard = SecureEraseGuard{digest};
    auto z = *curve.numberFromBigEndian(digest.span());
    const auto zEraseGuard = SecureEraseGuard{z};

    // FIPS 186-5 section 6.4.1 step 2 and RFC 6979 section 3.2: derive 1 <= k < n deterministically.
    auto k = deterministicNonce(d, digest.span(), curveName, hashAlgorithm);
    const auto kEraseGuard = SecureEraseGuard{k};

    // FIPS 186-5 section 6.4.1 step 3: (x_R,y_R) = [k]G and r = x_R mod n.
    auto point = curve.multiplyBaseSecret(k);
    const auto pointEraseGuard = SecureEraseGuard{point};
    auto affinePoint = curve.toAffineSecret(point);
    const auto affinePointEraseGuard = SecureEraseGuard{affinePoint};
    auto r = curve.reduceOrderSecret(affinePoint.x);
    const auto rEraseGuard = SecureEraseGuard{r};
    if (curve.isZeroSecret(r)) {
        throw CryptologyError{"Deterministic ECDSA produced the forbidden zero r component."_el};
    }

    // FIPS 186-5 section 6.4.1 step 4: s = k^-1 * (z + r*d) mod n.
    auto product = curve.multiplyOrderSecret(r, d);
    const auto productEraseGuard = SecureEraseGuard{product};
    auto sum = curve.addOrderSecret(z, product);
    const auto sumEraseGuard = SecureEraseGuard{sum};
    auto inverse = curve.invertOrderSecret(k);
    const auto inverseEraseGuard = SecureEraseGuard{inverse};
    auto s = curve.multiplyOrderSecret(inverse, sum);
    const auto sEraseGuard = SecureEraseGuard{s};
    if (curve.isZeroSecret(s)) {
        throw CryptologyError{"Deterministic ECDSA produced the forbidden zero s component."_el};
    }

    // RFC 3279 section 2.2.3: ECDSA-Sig-Value is a canonical DER SEQUENCE of positive r and s INTEGER values.
    auto encoder = DerEncoder{};
    const auto root = encoder.beginSequence();
    encoder.appendPositiveInteger(curve.numberToBigEndian(r).span());
    encoder.appendPositiveInteger(curve.numberToBigEndian(s).span());
    encoder.end(root);
    return encoder.encoded();
}

auto decodePrivateScalar(const mem::ConstByteSpan bytes, const NistPrimeCurve::Name curveName)
    -> NistPrimeCurve::Number {
    const auto curve = NistPrimeCurve{curveName};
    const auto scalar = curve.numberFromBigEndian(bytes);
    if (bytes.size() != curve.byteLength() || !scalar.has_value() || curve.isZero(*scalar) ||
        curve.compare(*scalar, curve.order()) >= 0) {
        throw err::ParameterError{
            "An EC private scalar must have the selected curve width and be in the range 1..n-1."_el,
            "privateScalar"_el};
    }
    return *scalar;
}

auto hashMessage(const mem::ConstByteSpan message, const HashAlgorithm hash) -> mem::ByteBlock {
    auto hasher = Hasher{hash};
    hasher.update(message);
    return hasher.finalize();
}

auto deterministicNonce(
    const NistPrimeCurve::Number &privateScalar,
    const mem::ConstByteSpan hash,
    const NistPrimeCurve::Name curveName,
    const HashAlgorithm hashAlgorithm) -> NistPrimeCurve::Number {
    const auto curve = NistPrimeCurve{curveName};
    auto x = curve.numberToBigEndian(privateScalar);
    x.markAsSensitive();
    const auto xEraseGuard = SecureEraseGuard{x};
    const auto hashNumber = *curve.numberFromBigEndian(hash);
    auto reducedHash = curve.numberToBigEndian(curve.reduceOrderSecret(hashNumber));
    reducedHash.markAsSensitive();
    const auto reducedHashEraseGuard = SecureEraseGuard{reducedHash};
    const auto digestLength = hashAlgorithm.digestSize();
    auto k = mem::ByteBlock{digestLength, mem::Byte{}};
    k.markAsSensitive();
    const auto kEraseGuard = SecureEraseGuard{k};
    auto v = mem::ByteBlock{digestLength, mem::Byte{1U}};
    v.markAsSensitive();
    const auto vEraseGuard = SecureEraseGuard{v};
    const auto zero = mem::ByteBlock{{mem::Byte{}}};
    const auto one = mem::ByteBlock{{mem::Byte{1U}}};

    // RFC 6979 section 3.2 steps d-f: initialize K and V, then incorporate int2octets(x) and bits2octets(h1).
    k = hmac(hashAlgorithm, k.span(), v.span(), zero.span(), x.span(), reducedHash.span());
    v = hmac(hashAlgorithm, k.span(), v.span());
    k = hmac(hashAlgorithm, k.span(), v.span(), one.span(), x.span(), reducedHash.span());
    v = hmac(hashAlgorithm, k.span(), v.span());

    // RFC 6979 section 3.2 step h: generate one 256-bit candidate at a time until 1 <= k < n.
    for (auto attempt = 0U; attempt < 128U; ++attempt) {
        auto candidateBytes = mem::ByteBlockEditor{};
        while (candidateBytes.length().toSizeT() < curve.byteLength()) {
            v = hmac(hashAlgorithm, k.span(), v.span());
            candidateBytes.append(v);
        }
        candidateBytes.markAsSensitive();
        const auto candidateBytesEraseGuard = SecureEraseGuard{candidateBytes};
        const auto candidate = curve.numberFromBigEndian(candidateBytes.span().first(curve.byteLength()));
        if (candidate.has_value() && !curve.isZero(*candidate) && curve.compare(*candidate, curve.order()) < 0) {
            return *candidate;
        }
        k = hmac(hashAlgorithm, k.span(), v.span(), zero.span());
        v = hmac(hashAlgorithm, k.span(), v.span());
    }
    throw CryptologyError{"RFC 6979 exceeded its fixed candidate bound."_el};
}

auto hmac(
    const HashAlgorithm hash,
    const mem::ConstByteSpan key,
    const mem::ConstByteSpan first,
    const mem::ConstByteSpan second,
    const mem::ConstByteSpan third,
    const mem::ConstByteSpan fourth) -> mem::ByteBlock {
    auto state = Hmac{hash, key};
    state.update(first);
    state.update(second);
    state.update(third);
    state.update(fourth);
    auto result = state.finalize();
    result.markAsSensitive();
    return result;
}

}
