// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Ed25519Signer.hpp"

#include "Ed25519Signature.hpp"

#include "../../../../err/ParameterError.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../HashAlgorithm.hpp"
#include "../../../Hasher.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::ed25519_signer {

using namespace text::literals;
using namespace unit;
using namespace ed25519_signature;

auto publicKey(const mem::ConstByteSpan seed) -> mem::ByteArray<32U> {
    if (seed.size() != 32U) {
        throw err::ParameterError{"An Ed25519 private seed must contain exactly 32 octets."_el, "seed"_el};
    }

    // RFC 8032 section 5.1.5 steps 1-2: h = SHA-512(seed), then prune the lower half into the secret scalar a.
    auto seedHasher = Hasher{HashAlgorithm::Sha2_512};
    seedHasher.update(seed);
    auto digest = seedHasher.finalize();
    digest.markAsSensitive();
    const auto digestEraseGuard = SecureEraseGuard{digest};
    auto scalar = expandedScalar(digest.span());
    const auto scalarEraseGuard = SecureEraseGuard{scalar};

    // RFC 8032 section 5.1.5 step 3: A = [a]B and publicKey = ENC(A).
    auto point = multiplyBaseSecret(scalar.span());
    const auto pointEraseGuard = SecureEraseGuard{point};
    return encodePoint(point);
}

auto sign(const mem::ConstByteSpan seed, const mem::ConstByteSpan message) -> mem::ByteBlock {
    if (seed.size() != 32U) {
        throw err::ParameterError{"An Ed25519 private seed must contain exactly 32 octets."_el, "seed"_el};
    }

    // RFC 8032 section 5.1.6 step 1: h = SHA-512(seed); its lower half yields a and its upper half is prefix.
    auto seedHasher = Hasher{HashAlgorithm::Sha2_512};
    seedHasher.update(seed);
    auto digest = seedHasher.finalize();
    digest.markAsSensitive();
    const auto digestEraseGuard = SecureEraseGuard{digest};
    auto secretScalar = expandedScalar(digest.span());
    const auto secretScalarEraseGuard = SecureEraseGuard{secretScalar};
    auto reducedSecretScalar = reduceScalar(secretScalar.span());
    const auto reducedSecretScalarEraseGuard = SecureEraseGuard{reducedSecretScalar};

    // RFC 8032 section 5.1.6 step 2: r = SHA-512(prefix || PH(M)) mod L; pure Ed25519 uses PH(M) = M.
    auto nonceHasher = Hasher{HashAlgorithm::Sha2_512};
    nonceHasher.update(digest.span().last(32U));
    nonceHasher.update(message);
    auto nonceDigest = nonceHasher.finalize();
    nonceDigest.markAsSensitive();
    const auto nonceDigestEraseGuard = SecureEraseGuard{nonceDigest};
    auto nonce = reduceScalar(nonceDigest.span());
    const auto nonceEraseGuard = SecureEraseGuard{nonce};

    // RFC 8032 section 5.1.6 step 3: R = [r]B and first = ENC(R).
    auto noncePoint = multiplyBaseSecret(nonce.span());
    const auto noncePointEraseGuard = SecureEraseGuard{noncePoint};
    const auto encodedNoncePoint = encodePoint(noncePoint);

    // RFC 8032 section 5.1.6 step 4: k = SHA-512(ENC(R) || ENC(A) || M) mod L.
    const auto encodedPublicKey = publicKey(seed);
    auto challengeHasher = Hasher{HashAlgorithm::Sha2_512};
    challengeHasher.update(encodedNoncePoint.span());
    challengeHasher.update(encodedPublicKey.span());
    challengeHasher.update(message);
    auto challengeDigest = challengeHasher.finalize();
    challengeDigest.markAsSensitive();
    const auto challengeDigestEraseGuard = SecureEraseGuard{challengeDigest};
    auto challenge = reduceScalar(challengeDigest.span());
    const auto challengeEraseGuard = SecureEraseGuard{challenge};

    // RFC 8032 section 5.1.6 step 5: S = (r + k*a) mod L.
    auto challengeProduct = multiplyScalars(challenge, reducedSecretScalar);
    const auto challengeProductEraseGuard = SecureEraseGuard{challengeProduct};
    auto response = addScalars(nonce, challengeProduct);
    const auto responseEraseGuard = SecureEraseGuard{response};

    // RFC 8032 section 5.1.6 step 6: return ENC(R) || ENC(S).
    auto signature = mem::ByteBlockEditor{};
    signature.append(encodedNoncePoint.span());
    signature.append(response.span());
    return mem::ByteBlock{signature};
}

auto expandedScalar(const mem::ConstByteSpan digest) -> Scalar {
    auto result = Scalar::fromSpanOrThrow(digest.first(32U));
    result.set(ByteIndex{0U}, mem::Byte{static_cast<uint8_t>(result.get(ByteIndex{0U}).toUInt8() & 248U)});
    result.set(ByteIndex{31U}, mem::Byte{static_cast<uint8_t>((result.get(ByteIndex{31U}).toUInt8() & 63U) | 64U)});
    return result;
}

auto multiplyBaseSecret(const mem::ConstByteSpan scalar) noexcept -> Point {
    auto result = identity();
    const auto point = basePoint();
    // RFC 8032 section 5.1.5 notation [a]B: execute one doubling and one addition for every one of 256 scalar bits.
    for (auto position = 256U; position-- > 0U;) {
        const auto doubled = doublePoint(result);
        const auto added = add(doubled, point);
        const auto bit = uint64_t{(scalar[position / 8U].toUInt8() >> (position & 7U)) & 1U};
        result = selectPoint(doubled, added, bit);
    }
    return result;
}

auto selectPoint(const Point &first, const Point &second, const uint64_t selectSecond) noexcept -> Point {
    return Point{
        FieldElement::select(first.x, second.x, selectSecond),
        FieldElement::select(first.y, second.y, selectSecond),
        FieldElement::select(first.z, second.z, selectSecond),
        FieldElement::select(first.t, second.t, selectSecond)};
}

auto encodePoint(const Point &point) noexcept -> Scalar {
    // RFC 8032 section 5.1.2: recover affine x=X/Z and y=Y/Z, encode y, then store x mod 2 in the top bit.
    const auto inverseZ = FieldElement::invert(point.z);
    const auto x = FieldElement::multiply(point.x, inverseZ);
    const auto y = FieldElement::multiply(point.y, inverseZ);
    auto result = y.toBytes();
    const auto sign = static_cast<uint8_t>(x.isNegative() ? 0x80U : 0U);
    result.set(ByteIndex{31U}, result.get(ByteIndex{31U}) | mem::Byte{sign});
    return result;
}

auto reduceScalar(const mem::ConstByteSpan value) noexcept -> Scalar {
    auto result = Scalar{};
    const auto bitCount = value.size() * 8U;
    // RFC 8032 section 5.1.6 interprets SHA-512 output little-endian modulo L. Fixed bitwise reduction keeps every
    // intermediate below L and uses masked subtraction independent of the digest value.
    for (auto position = bitCount; position-- > 0U;) {
        shiftAppendReduce(result, static_cast<uint8_t>((value[position / 8U].toUInt8() >> (position & 7U)) & 1U));
    }
    return result;
}

auto addScalars(const Scalar &left, const Scalar &right) noexcept -> Scalar {
    auto result = left;
    auto carry = uint16_t{};
    // RFC 8032 section 5.1.6 step 5: both operands are below L, so their sum is below 2L and needs one reduction.
    for (auto index = std::size_t{}; index < 32U; ++index) {
        const auto value = static_cast<uint16_t>(result.get(ByteIndex::fromSizeT(index)).toUInt8()) +
            right.get(ByteIndex::fromSizeT(index)).toUInt8() + carry;
        result.set(ByteIndex::fromSizeT(index), mem::Byte{static_cast<uint8_t>(value)});
        carry = static_cast<uint16_t>(value >> 8U);
    }
    reduceOnce(result);
    return result;
}

auto multiplyScalars(const Scalar &left, const Scalar &right) noexcept -> Scalar {
    auto result = Scalar{};
    auto addend = left;
    for (auto position = std::size_t{}; position < 256U; ++position) {
        const auto bit =
            static_cast<uint8_t>((right.get(ByteIndex::fromSizeT(position / 8U)).toUInt8() >> (position & 7U)) & 1U);
        auto candidate = result;
        auto carry = uint16_t{};
        for (auto index = std::size_t{}; index < 32U; ++index) {
            const auto value = static_cast<uint16_t>(candidate.get(ByteIndex::fromSizeT(index)).toUInt8()) +
                addend.get(ByteIndex::fromSizeT(index)).toUInt8() + carry;
            candidate.set(ByteIndex::fromSizeT(index), mem::Byte{static_cast<uint8_t>(value)});
            carry = static_cast<uint16_t>(value >> 8U);
        }
        reduceOnce(candidate);
        const auto mask = static_cast<uint8_t>(0U - bit);
        for (auto index = std::size_t{}; index < 32U; ++index) {
            const auto current = result.get(ByteIndex::fromSizeT(index)).toUInt8();
            const auto replacement = candidate.get(ByteIndex::fromSizeT(index)).toUInt8();
            result.set(
                ByteIndex::fromSizeT(index),
                mem::Byte{static_cast<uint8_t>((current & static_cast<uint8_t>(~mask)) | (replacement & mask))});
        }
        shiftAppendReduce(addend, 0U);
    }
    return result;
}

void shiftAppendReduce(Scalar &value, const uint8_t bit) noexcept {
    auto carry = bit;
    for (auto index = std::size_t{}; index < 32U; ++index) {
        const auto current = value.get(ByteIndex::fromSizeT(index)).toUInt8();
        value.set(ByteIndex::fromSizeT(index), mem::Byte{static_cast<uint8_t>((current << 1U) | carry)});
        carry = static_cast<uint8_t>(current >> 7U);
    }
    reduceOnce(value);
}

void reduceOnce(Scalar &value) noexcept {
    static constexpr auto order = std::array<uint8_t, 32U>{
        0xedU,
        0xd3U,
        0xf5U,
        0x5cU,
        0x1aU,
        0x63U,
        0x12U,
        0x58U,
        0xd6U,
        0x9cU,
        0xf7U,
        0xa2U,
        0xdeU,
        0xf9U,
        0xdeU,
        0x14U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x00U,
        0x10U};
    auto difference = Scalar{};
    auto borrow = uint16_t{};
    for (auto index = std::size_t{}; index < 32U; ++index) {
        const auto current = uint16_t{value.get(ByteIndex::fromSizeT(index)).toUInt8()};
        const auto subtrahend = static_cast<uint16_t>(order[index]) + borrow;
        difference.set(ByteIndex::fromSizeT(index), mem::Byte{static_cast<uint8_t>(current - subtrahend)});
        borrow = current < subtrahend ? 1U : 0U;
    }
    const auto useDifferenceMask = static_cast<uint8_t>(0U - static_cast<uint8_t>(borrow ^ 1U));
    for (auto index = std::size_t{}; index < 32U; ++index) {
        const auto original = value.get(ByteIndex::fromSizeT(index)).toUInt8();
        const auto reduced = difference.get(ByteIndex::fromSizeT(index)).toUInt8();
        value.set(
            ByteIndex::fromSizeT(index),
            mem::Byte{static_cast<uint8_t>(
                (original & static_cast<uint8_t>(~useDifferenceMask)) | (reduced & useDifferenceMask))});
    }
}

}
