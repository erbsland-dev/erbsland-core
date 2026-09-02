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

using namespace mem;
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

[[nodiscard]] auto encodePss(
    const HashAlgorithm hash, const ConstByteSpan message, const ConstByteSpan salt, const std::size_t emBits)
    -> ByteBlock {
    const auto hashLength = hash.digestSize().toSizeT();
    const auto emLength = (emBits + 7U) / 8U;
    if (salt.size() != hashLength || emLength < hashLength + salt.size() + 2U) {
        throw err::ParameterError{"RSA-PSS randomness has an invalid length for this key and hash."_el, "salt"_el};
    }

    // RFC 8017 section 9.1.1 steps 1-2: mHash = Hash(M), with digest-sized salt fixed by RFC 8446 section 4.2.3.
    auto messageHash = rsa_signature::hashMessage(hash, message);
    const auto messageHashEraseGuard = SecureEraseGuard{messageHash};

    // RFC 8017 section 9.1.1 step 3: M' = 0x00^8 || mHash || salt.
    auto hashInput = ByteBlockEditor{ByteLength{8U}};
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
    auto db = ByteBlockEditor{ByteLength::fromSizeT(psLength)};
    db.markAsSensitive();
    const auto dbEraseGuard = SecureEraseGuard{db};
    db.append(Byte{1U});
    db.append(salt);

    // RFC 8017 section 9.1.1 steps 7-9: maskedDB = DB xor MGF1(H), then clear unused high bits.
    auto dbMask = rsa_signature::mgf1(hash, encodedHash.span(), db.length().toSizeT());
    dbMask.markAsSensitive();
    const auto dbMaskEraseGuard = SecureEraseGuard{dbMask};
    db.xorWithOrThrow(dbMask);
    const auto unusedHighBits = 8U * emLength - emBits;
    db.set(
        ByteIndex::zero(), Byte{static_cast<uint8_t>(db.get(ByteIndex::zero()).toUInt8() & (0xffU >> unusedHighBits))});

    // RFC 8017 section 9.1.1 step 10: EM = maskedDB || H || 0xbc.
    auto encodedMessage = ByteBlockEditor{};
    encodedMessage.reserve(ByteLength::fromSizeT(emLength));
    encodedMessage.append(db);
    encodedMessage.append(encodedHash);
    encodedMessage.append(Byte{0xbcU});
    auto result = ByteBlock{encodedMessage};
    result.markAsSensitive();
    return result;
}

[[nodiscard]] auto crtExponentiation(const PrivateKeyData &key, const Number &value) noexcept -> Number {
    // RFC 8017 section 5.1.2 step 2.b.i: m1 = c^dP mod p and m2 = c^dQ mod q.
    auto valueP = value.reducedSecret(key.prime1);
    const auto valuePEraseGuard = SecureEraseGuard{valueP};
    auto valueQ = value.reducedSecret(key.prime2);
    const auto valueQEraseGuard = SecureEraseGuard{valueQ};
    auto m1 = valueP.poweredModuloSecret(key.exponent1, key.prime1);
    const auto m1EraseGuard = SecureEraseGuard{m1};
    auto m2 = valueQ.poweredModuloSecret(key.exponent2, key.prime2);
    const auto m2EraseGuard = SecureEraseGuard{m2};

    // RFC 8017 section 5.1.2 step 2.b.ii: h = (m1 - m2) * qInv mod p.
    auto m2ModuloP = m2.reducedSecret(key.prime1);
    const auto m2ModuloPEraseGuard = SecureEraseGuard{m2ModuloP};
    auto difference = m1.subtractedModuloSecret(m2ModuloP, key.prime1);
    const auto differenceEraseGuard = SecureEraseGuard{difference};
    auto h = difference.multipliedModuloSecret(key.coefficient, key.prime1);
    const auto hEraseGuard = SecureEraseGuard{h};

    // RFC 8017 section 5.1.2 step 2.b.iii: m = m2 + q*h, represented at the public modulus width.
    auto qh = key.prime2.multiplied(h, key.modulus.wordCount());
    const auto qhEraseGuard = SecureEraseGuard{qh};
    auto m2Padded = m2.padded(key.modulus.wordCount());
    const auto m2PaddedEraseGuard = SecureEraseGuard{m2Padded};
    return m2Padded.added(qh, key.modulus.wordCount());
}

[[nodiscard]] auto signDecoded(
    const PrivateKeyData &key,
    const TlsSignatureScheme scheme,
    const ConstByteSpan message,
    const PublicKey &publicKeyValue,
    const ConstByteSpan salt,
    const ConstByteSpan blindingFactor) -> ByteBlock {
    const auto hash = hashForScheme(scheme);
    auto encodedMessage = encodePss(hash, message, salt, key.modulusBits - 1U);
    const auto encodedMessageEraseGuard = SecureEraseGuard{encodedMessage};
    auto messageRepresentative = Number::fromBigEndian(encodedMessage.span(), key.modulus.wordCount());
    const auto messageRepresentativeEraseGuard = SecureEraseGuard{messageRepresentative};

    // RFC 8017 section 5.1.2: sample r in [1,n-1]; it is owned by guarded scratch from conversion onward.
    if (blindingFactor.size() != key.encodedLength) {
        throw err::ParameterError{"RSA blinding randomness must have the modulus length."_el, "blindingFactor"_el};
    }
    auto r = Number::fromBigEndian(blindingFactor, key.modulus.wordCount());
    const auto rEraseGuard = SecureEraseGuard{r};
    if (r.isZero() || r.compare(key.modulus) >= 0) {
        throw CryptologyError{"RSA blinding randomness is not in the interval 1..n-1."_el};
    }

    // RSA blinding: c' = c * r^e mod n. The exponent schedule is fixed to the modulus width even though e is public.
    auto e = Number::fromBigEndian(key.publicExponent.span(), key.modulus.wordCount());
    const auto eEraseGuard = SecureEraseGuard{e};
    auto rPowerE = r.poweredModuloSecret(e, key.modulus);
    const auto rPowerEraseGuard = SecureEraseGuard{rPowerE};
    auto blindedMessage = messageRepresentative.multipliedModuloSecret(rPowerE, key.modulus);
    const auto blindedMessageEraseGuard = SecureEraseGuard{blindedMessage};

    // RFC 8017 appendix A.1.2: phi(n)=(p-1)(q-1), so r^(phi(n)-1) is r^-1 for a valid blinding factor.
    auto pMinusOne = key.prime1.subtractOne();
    const auto pMinusOneEraseGuard = SecureEraseGuard{pMinusOne};
    auto qMinusOne = key.prime2.subtractOne();
    const auto qMinusOneEraseGuard = SecureEraseGuard{qMinusOne};
    auto phi = pMinusOne.multiplied(qMinusOne, key.modulus.wordCount());
    const auto phiEraseGuard = SecureEraseGuard{phi};
    auto inverseExponent = phi.subtractOne();
    const auto inverseExponentEraseGuard = SecureEraseGuard{inverseExponent};
    auto inverseR = r.poweredModuloSecret(inverseExponent, key.modulus);
    const auto inverseREraseGuard = SecureEraseGuard{inverseR};

    // RFC 8017 section 5.1.2: apply the two-prime CRT acceleration to the blinded representative.
    auto blindedSignature = crtExponentiation(key, blindedMessage);
    const auto blindedSignatureEraseGuard = SecureEraseGuard{blindedSignature};
    auto signatureRepresentative = blindedSignature.multipliedModuloSecret(inverseR, key.modulus);
    const auto signatureRepresentativeEraseGuard = SecureEraseGuard{signatureRepresentative};
    const auto signature = signatureRepresentative.toBigEndian(key.encodedLength);

    // Mandatory fault defense: public RSAVP1 and EMSA-PSS verification must accept every result before release.
    if (!publicKeyValue.verifyTlsCertificateVerifySignature(scheme, message, signature.span())) {
        throw CryptologyError{"RSA-PSS signing result failed its mandatory public-key verification."_el};
    }
    return signature;
}

}

namespace erbsland::cryptology::impl::rsa_signer {

using namespace mem;
using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

auto sign(
    const ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const ConstByteSpan message,
    const PublicKey &publicKeyValue) -> ByteBlock {
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
        auto r = Number::fromBigEndian(blindingFactor.span(), key.modulus.wordCount());
        const auto rEraseGuard = SecureEraseGuard{r};
        if (!r.isZero() && r.compare(key.modulus) < 0) {
            return signing::signDecoded(key, scheme, message, publicKeyValue, salt.span(), blindingFactor.span());
        }
    }
    throw CryptologyError{"Secure randomness did not produce a valid RSA blinding factor within the fixed bound."_el};
}

auto signWithRandom(
    const ConstByteSpan privateKey,
    const TlsSignatureScheme scheme,
    const ConstByteSpan message,
    const PublicKey &publicKeyValue,
    const ConstByteSpan salt,
    const ConstByteSpan blindingFactor) -> ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};
    return signing::signDecoded(key, scheme, message, publicKeyValue, salt, blindingFactor);
}

}
