// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSigner.hpp"

#include "../../../../core/Application.hpp"
#include "../../../../err/ParameterError.hpp"
#include "../../../../random/Random.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../CryptologyError.hpp"
#include "../../../Hasher.hpp"
#include "../../../tls/TlsSignatureScheme.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::rsa_signer::signing {

using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

[[nodiscard]] auto hashForScheme(const TlsSignatureScheme scheme) -> HashAlgorithm {
    if (scheme == TlsSignatureScheme::RsaPssRsaeSha256 || scheme == TlsSignatureScheme::RsaPssPssSha256) {
        return HashAlgorithm::Sha2_256;
    }
    if (scheme == TlsSignatureScheme::RsaPssRsaeSha384 || scheme == TlsSignatureScheme::RsaPssPssSha384) {
        return HashAlgorithm::Sha2_384;
    }
    throw err::ParameterError{"RSA signing requires a TLS 1.3 RSA-PSS scheme."_el, "scheme"_el};
}

[[nodiscard]] auto subtractOne(Number value) noexcept -> Number {
    auto borrow = uint64_t{1U};
    for (auto index = std::size_t{}; index < value.count; ++index) {
        const auto current = uint64_t{value.words[index]};
        value.words[index] = static_cast<uint32_t>(current - borrow);
        borrow = current < borrow ? 1U : 0U;
    }
    return value;
}

[[nodiscard]] auto isZero(const Number &value) noexcept -> bool {
    auto combined = uint32_t{};
    for (auto index = std::size_t{}; index < value.count; ++index) {
        combined |= value.words[index];
    }
    return combined == 0U;
}

[[nodiscard]] auto addExact(const Number &left, const Number &right, const std::size_t count) noexcept -> Number {
    auto result = Number{};
    result.count = count;
    auto carry = uint64_t{};
    for (auto index = std::size_t{}; index < count; ++index) {
        const auto word = uint64_t{left.words[index]} + right.words[index] + carry;
        result.words[index] = static_cast<uint32_t>(word);
        carry = word >> 32U;
    }
    return result;
}

[[nodiscard]] auto encodePss(
    const HashAlgorithm hash, const mem::ConstByteSpan message, const mem::ConstByteSpan salt, const std::size_t emBits)
    -> mem::ByteBlock {
    const auto hashLength = hash.digestSize().toSizeT();
    const auto emLength = (emBits + 7U) / 8U;
    if (salt.size() != hashLength || emLength < hashLength + salt.size() + 2U) {
        throw err::ParameterError{"RSA-PSS randomness has an invalid length for this key and hash."_el, "salt"_el};
    }

    // RFC 8017 section 9.1.1 steps 1-2: mHash = Hash(M), with digest-sized salt fixed by RFC 8446 section 4.2.3.
    auto messageHash = rsa_signature::hashMessage(hash, message);
    const auto messageHashEraseGuard = SecureEraseGuard{messageHash};

    // RFC 8017 section 9.1.1 step 3: M' = 0x00^8 || mHash || salt.
    auto hashInput = mem::ByteBlockEditor{ByteLength{8U}};
    hashInput.markAsSensitive();
    const auto hashInputEraseGuard = SecureEraseGuard{hashInput};
    hashInput.append(messageHash);
    hashInput.append(salt);

    // RFC 8017 section 9.1.1 step 4: H = Hash(M').
    auto encodedHash = rsa_signature::hashMessage(hash, hashInput.span());
    encodedHash.markAsSensitive();
    const auto encodedHashEraseGuard = SecureEraseGuard{encodedHash};

    // RFC 8017 section 9.1.1 steps 5-6: DB = PS || 0x01 || salt.
    const auto psLength = emLength - salt.size() - hashLength - 2U;
    auto db = mem::ByteBlockEditor{ByteLength::fromSizeT(psLength)};
    db.markAsSensitive();
    const auto dbEraseGuard = SecureEraseGuard{db};
    db.append(mem::Byte{1U});
    db.append(salt);

    // RFC 8017 section 9.1.1 steps 7-9: maskedDB = DB xor MGF1(H), then clear unused high bits.
    auto dbMask = rsa_signature::mgf1(hash, encodedHash.span(), db.length().toSizeT());
    dbMask.markAsSensitive();
    const auto dbMaskEraseGuard = SecureEraseGuard{dbMask};
    db.xorWithOrThrow(dbMask);
    const auto unusedHighBits = 8U * emLength - emBits;
    db.set(
        ByteIndex::zero(),
        mem::Byte{static_cast<uint8_t>(db.get(ByteIndex::zero()).toUInt8() & (0xffU >> unusedHighBits))});

    // RFC 8017 section 9.1.1 step 10: EM = maskedDB || H || 0xbc.
    auto encodedMessage = mem::ByteBlockEditor{};
    encodedMessage.reserve(ByteLength::fromSizeT(emLength));
    encodedMessage.append(db);
    encodedMessage.append(encodedHash);
    encodedMessage.append(mem::Byte{0xbcU});
    auto result = mem::ByteBlock{encodedMessage};
    result.markAsSensitive();
    return result;
}

[[nodiscard]] auto crtExponentiation(const PrivateKeyData &key, const Number &value) noexcept -> Number {
    // RFC 8017 section 5.1.2 step 2.b.i: m1 = c^dP mod p and m2 = c^dQ mod q.
    auto valueP = reduceSecret(value, key.prime1);
    const auto valuePEraseGuard = SecureEraseGuard{valueP};
    auto valueQ = reduceSecret(value, key.prime2);
    const auto valueQEraseGuard = SecureEraseGuard{valueQ};
    auto m1 = powerModuloSecret(valueP, key.exponent1, key.prime1);
    const auto m1EraseGuard = SecureEraseGuard{m1};
    auto m2 = powerModuloSecret(valueQ, key.exponent2, key.prime2);
    const auto m2EraseGuard = SecureEraseGuard{m2};

    // RFC 8017 section 5.1.2 step 2.b.ii: h = (m1 - m2) * qInv mod p.
    auto m2ModuloP = reduceSecret(m2, key.prime1);
    const auto m2ModuloPEraseGuard = SecureEraseGuard{m2ModuloP};
    auto difference = subtractModuloSecret(m1, m2ModuloP, key.prime1);
    const auto differenceEraseGuard = SecureEraseGuard{difference};
    auto h = multiplyModuloSecret(difference, key.coefficient, key.prime1);
    const auto hEraseGuard = SecureEraseGuard{h};

    // RFC 8017 section 5.1.2 step 2.b.iii: m = m2 + q*h, represented at the public modulus width.
    auto qh = multiplyExact(key.prime2, h, key.modulus.count);
    const auto qhEraseGuard = SecureEraseGuard{qh};
    auto m2Padded = m2;
    m2Padded.count = key.modulus.count;
    const auto m2PaddedEraseGuard = SecureEraseGuard{m2Padded};
    return addExact(m2Padded, qh, key.modulus.count);
}

[[nodiscard]] auto signDecoded(
    const PrivateKeyData &key,
    const TlsSignatureScheme scheme,
    const mem::ConstByteSpan message,
    const PublicKey &publicKeyValue,
    const mem::ConstByteSpan salt,
    const mem::ConstByteSpan blindingFactor) -> mem::ByteBlock {
    const auto hash = hashForScheme(scheme);
    auto encodedMessage = encodePss(hash, message, salt, key.modulusBits - 1U);
    const auto encodedMessageEraseGuard = SecureEraseGuard{encodedMessage};
    auto messageRepresentative = rsa_signature::numberFromBigEndian(encodedMessage.span(), key.modulus.count);
    const auto messageRepresentativeEraseGuard = SecureEraseGuard{messageRepresentative};

    // RFC 8017 section 5.1.2: sample r in [1,n-1]; it is owned by guarded scratch from conversion onward.
    if (blindingFactor.size() != key.encodedLength) {
        throw err::ParameterError{"RSA blinding randomness must have the modulus length."_el, "blindingFactor"_el};
    }
    auto r = rsa_signature::numberFromBigEndian(blindingFactor, key.modulus.count);
    const auto rEraseGuard = SecureEraseGuard{r};
    if (isZero(r) || rsa_signature::compare(r, key.modulus) >= 0) {
        throw CryptologyError{"RSA blinding randomness is not in the interval 1..n-1."_el};
    }

    // RSA blinding: c' = c * r^e mod n. The exponent schedule is fixed to the modulus width even though e is public.
    auto e = rsa_signature::numberFromBigEndian(key.publicExponent.span(), key.modulus.count);
    const auto eEraseGuard = SecureEraseGuard{e};
    auto rPowerE = powerModuloSecret(r, e, key.modulus);
    const auto rPowerEraseGuard = SecureEraseGuard{rPowerE};
    auto blindedMessage = multiplyModuloSecret(messageRepresentative, rPowerE, key.modulus);
    const auto blindedMessageEraseGuard = SecureEraseGuard{blindedMessage};

    // RFC 8017 appendix A.1.2: phi(n)=(p-1)(q-1), so r^(phi(n)-1) is r^-1 for a valid blinding factor.
    auto pMinusOne = subtractOne(key.prime1);
    const auto pMinusOneEraseGuard = SecureEraseGuard{pMinusOne};
    auto qMinusOne = subtractOne(key.prime2);
    const auto qMinusOneEraseGuard = SecureEraseGuard{qMinusOne};
    auto phi = multiplyExact(pMinusOne, qMinusOne, key.modulus.count);
    const auto phiEraseGuard = SecureEraseGuard{phi};
    auto inverseExponent = subtractOne(phi);
    const auto inverseExponentEraseGuard = SecureEraseGuard{inverseExponent};
    auto inverseR = powerModuloSecret(r, inverseExponent, key.modulus);
    const auto inverseREraseGuard = SecureEraseGuard{inverseR};

    // RFC 8017 section 5.1.2: apply the two-prime CRT acceleration to the blinded representative.
    auto blindedSignature = crtExponentiation(key, blindedMessage);
    const auto blindedSignatureEraseGuard = SecureEraseGuard{blindedSignature};
    auto signatureRepresentative = multiplyModuloSecret(blindedSignature, inverseR, key.modulus);
    const auto signatureRepresentativeEraseGuard = SecureEraseGuard{signatureRepresentative};
    const auto signature = rsa_signature::numberToBigEndian(signatureRepresentative, key.encodedLength);

    // Mandatory fault defense: public RSAVP1 and EMSA-PSS verification must accept every result before release.
    if (!publicKeyValue.verifyTlsCertificateVerifySignature(scheme, message, signature.span())) {
        throw CryptologyError{"RSA-PSS signing result failed its mandatory public-key verification."_el};
    }
    return signature;
}

}

namespace erbsland::cryptology::impl::rsa_signer {

using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

auto sign(
    const mem::ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const mem::ConstByteSpan message,
    const PublicKey &publicKeyValue) -> mem::ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};
    const auto hash = signing::hashForScheme(scheme);

    // RFC 8446 section 4.2.3: use a digest-sized random PSS salt and one modulus-sized message-blinding factor.
    auto salt = core::application().secureRandom().buildByteBlock(hash.digestSize());
    salt.markAsSensitive();
    const auto saltEraseGuard = SecureEraseGuard{salt};
    for (auto attempt = 0U; attempt < 64U; ++attempt) {
        auto blindingFactor =
            core::application().secureRandom().buildByteBlock(ByteLength::fromSizeT(key.encodedLength));
        blindingFactor.markAsSensitive();
        const auto blindingFactorEraseGuard = SecureEraseGuard{blindingFactor};
        auto r = rsa_signature::numberFromBigEndian(blindingFactor.span(), key.modulus.count);
        const auto rEraseGuard = SecureEraseGuard{r};
        if (!signing::isZero(r) && rsa_signature::compare(r, key.modulus) < 0) {
            return signing::signDecoded(key, scheme, message, publicKeyValue, salt.span(), blindingFactor.span());
        }
    }
    throw CryptologyError{"Secure randomness did not produce a valid RSA blinding factor within the fixed bound."_el};
}

auto signWithRandom(
    const mem::ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const mem::ConstByteSpan message,
    const PublicKey &publicKeyValue,
    const mem::ConstByteSpan salt,
    const mem::ConstByteSpan blindingFactor) -> mem::ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};
    return signing::signDecoded(key, scheme, message, publicKeyValue, salt, blindingFactor);
}

}
