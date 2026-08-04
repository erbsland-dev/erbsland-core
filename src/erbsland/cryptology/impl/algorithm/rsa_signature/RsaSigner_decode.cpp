// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSigner.hpp"

#include "../../../../err/ParseError.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ItemIndex.hpp"
#include "../../DerEncoder.hpp"
#include "../../DerParser.hpp"
#include "../../SecureEraseGuard.hpp"

#include <algorithm>
#include <bit>

namespace erbsland::cryptology::impl::rsa_signer::decoder {

using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

[[noreturn]] void fail(const text::String &reason) {
    throw err::ParseError{reason};
}

[[nodiscard]] auto equal(const Number &left, const Number &right, const std::size_t count) noexcept -> bool {
    auto difference = uint32_t{};
    for (auto index = std::size_t{}; index < count; ++index) {
        difference |= left.words[index] ^ right.words[index];
    }
    return difference == 0U;
}

[[nodiscard]] auto isOne(const Number &value) noexcept -> bool {
    auto difference = value.words[0U] ^ 1U;
    for (auto index = std::size_t{1U}; index < value.count; ++index) {
        difference |= value.words[index];
    }
    return difference == 0U;
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

[[nodiscard]] auto padded(const Number &value, const std::size_t count) noexcept -> Number {
    auto result = value;
    result.count = count;
    return result;
}

[[nodiscard]] auto parseInteger(
    const Asn1Node &node, const text::String &name, const std::size_t count, const bool sensitive) -> Number {
    auto bytes = rsa_signature::decodePositiveInteger(node, name);
    if (sensitive) {
        bytes.markAsSensitive();
    }
    const auto bytesEraseGuard = SecureEraseGuard{bytes};
    return rsa_signature::numberFromBigEndian(bytes.span(), count);
}

}

namespace erbsland::cryptology::impl::rsa_signer {

using namespace text::literals;
using namespace unit;
using rsa_signature::Number;

auto decodePrivateKey(const mem::ConstByteSpan privateKey) -> PrivateKeyData {
    auto encoded = mem::ByteBlock::fromSpan(privateKey);
    encoded.markAsSensitive();
    const auto encodedEraseGuard = SecureEraseGuard{encoded};

    // RFC 8017 appendix A.1.2: RSAPrivateKey contains version plus exactly eight two-prime integer components.
    const auto root = DerParser{encoded}.parseDocument();
    rsa_signature::requireNode(
        root, Asn1TagClass::Universal, static_cast<uint32_t>(Asn1UniversalType::Sequence), true, "RSAPrivateKey"_el);
    if (root.childCount() != ItemCount{9U} ||
        rsa_signature::decodeSmallInteger(root.child(ItemIndex{0U}), "version"_el) != 0U) {
        decoder::fail("Only RFC 8017 two-prime RSAPrivateKey version 0 is supported."_el);
    }

    auto result = PrivateKeyData{};
    auto resultEraseGuard = SecureEraseGuard{result};
    auto modulusBytes = rsa_signature::decodePositiveInteger(root.child(ItemIndex{1U}), "modulus"_el);
    const auto firstModulusByte = modulusBytes.span().front().toUInt8();
    result.modulusBits =
        (modulusBytes.span().size() - 1U) * 8U + static_cast<std::size_t>(std::bit_width(firstModulusByte));
    if (result.modulusBits < rsa_signature::cMinimumModulusBits ||
        result.modulusBits > rsa_signature::cMaximumModulusBits || (modulusBytes.span().back().toUInt8() & 1U) == 0U) {
        decoder::fail("The RSA modulus must be odd and between 2048 and 8192 bits."_el);
    }
    result.encodedLength = (result.modulusBits + 7U) / 8U;
    const auto modulusWords = (result.modulusBits + 31U) / 32U;
    result.modulus = rsa_signature::numberFromBigEndian(modulusBytes.span(), modulusWords);

    // FIPS 186-5 section 5.1: e is odd, greater than 2^16, and bounded here to 256 bits.
    result.publicExponent = rsa_signature::decodePositiveInteger(root.child(ItemIndex{2U}), "publicExponent"_el);
    if (result.publicExponent.span().size() > rsa_signature::cMaximumExponentBytes ||
        (result.publicExponent.span().back().toUInt8() & 1U) == 0U) {
        decoder::fail("The RSA public exponent is outside the fixed 17--256-bit odd bound."_el);
    }
    const auto exponentIsLargeEnough = result.publicExponent.span().size() > 3U ||
        (result.publicExponent.span().size() == 3U &&
            (result.publicExponent.span()[0U].toUInt8() > 1U || result.publicExponent.span()[1U].toUInt8() != 0U ||
                result.publicExponent.span()[2U].toUInt8() != 0U));
    if (!exponentIsLargeEnough) {
        decoder::fail("The RSA public exponent must be greater than 65536."_el);
    }

    // RFC 8017 appendix A.1.2: decode d, p, q, dP, dQ, and qInv into bounded scratch that the guard erases.
    result.privateExponent = decoder::parseInteger(root.child(ItemIndex{3U}), "privateExponent"_el, modulusWords, true);
    auto prime1Bytes = rsa_signature::decodePositiveInteger(root.child(ItemIndex{4U}), "prime1"_el);
    prime1Bytes.markAsSensitive();
    const auto prime1BytesEraseGuard = SecureEraseGuard{prime1Bytes};
    auto prime2Bytes = rsa_signature::decodePositiveInteger(root.child(ItemIndex{5U}), "prime2"_el);
    prime2Bytes.markAsSensitive();
    const auto prime2BytesEraseGuard = SecureEraseGuard{prime2Bytes};
    const auto prime1Words = (prime1Bytes.span().size() + 3U) / 4U;
    const auto prime2Words = (prime2Bytes.span().size() + 3U) / 4U;
    if (prime1Words == 0U || prime2Words == 0U || prime1Words + prime2Words < modulusWords) {
        decoder::fail("RSA prime factors are inconsistent with the modulus size."_el);
    }
    result.prime1 = rsa_signature::numberFromBigEndian(prime1Bytes.span(), prime1Words);
    result.prime2 = rsa_signature::numberFromBigEndian(prime2Bytes.span(), prime2Words);
    if ((result.prime1.words[0U] & 1U) == 0U || (result.prime2.words[0U] & 1U) == 0U) {
        decoder::fail("RSA prime factors must be odd."_el);
    }
    result.exponent1 = decoder::parseInteger(root.child(ItemIndex{6U}), "exponent1"_el, prime1Words, true);
    result.exponent2 = decoder::parseInteger(root.child(ItemIndex{7U}), "exponent2"_el, prime2Words, true);
    result.coefficient = decoder::parseInteger(root.child(ItemIndex{8U}), "coefficient"_el, prime1Words, true);

    // RFC 8017 appendix A.1.2: n must equal p*q exactly; version 0 excludes OtherPrimeInfos.
    auto product = multiplyExact(result.prime1, result.prime2, modulusWords);
    const auto productEraseGuard = SecureEraseGuard{product};
    if (!decoder::equal(product, result.modulus, modulusWords)) {
        decoder::fail("RSA modulus does not equal prime1 multiplied by prime2."_el);
    }

    auto pMinusOne = decoder::subtractOne(result.prime1);
    const auto pMinusOneEraseGuard = SecureEraseGuard{pMinusOne};
    auto qMinusOne = decoder::subtractOne(result.prime2);
    const auto qMinusOneEraseGuard = SecureEraseGuard{qMinusOne};
    auto publicExponentPadded =
        rsa_signature::numberFromBigEndian(result.publicExponent.span(), std::max(prime1Words, prime2Words));
    const auto publicExponentEraseGuard = SecureEraseGuard{publicExponentPadded};

    // RFC 8017 appendix A.1.2: dP=d mod(p-1), dQ=d mod(q-1), and each reduced exponent inverts e.
    auto reducedD1 = reduceSecret(result.privateExponent, pMinusOne);
    const auto reducedD1EraseGuard = SecureEraseGuard{reducedD1};
    auto reducedD2 = reduceSecret(result.privateExponent, qMinusOne);
    const auto reducedD2EraseGuard = SecureEraseGuard{reducedD2};
    if (!decoder::equal(reducedD1, result.exponent1, prime1Words) ||
        !decoder::equal(reducedD2, result.exponent2, prime2Words)) {
        decoder::fail("RSA CRT exponents do not match the private exponent."_el);
    }
    auto e1 = decoder::padded(publicExponentPadded, prime1Words);
    auto e2 = decoder::padded(publicExponentPadded, prime2Words);
    auto inverseCheck1 = multiplyModuloSecret(e1, result.exponent1, pMinusOne);
    const auto inverseCheck1EraseGuard = SecureEraseGuard{inverseCheck1};
    auto inverseCheck2 = multiplyModuloSecret(e2, result.exponent2, qMinusOne);
    const auto inverseCheck2EraseGuard = SecureEraseGuard{inverseCheck2};
    if (!decoder::isOne(inverseCheck1) || !decoder::isOne(inverseCheck2)) {
        decoder::fail("RSA CRT exponents are inconsistent with the public exponent."_el);
    }

    // RFC 8017 appendix A.1.2: qInv*q mod p must equal one.
    auto qModuloP = reduceSecret(result.prime2, result.prime1);
    const auto qModuloPEraseGuard = SecureEraseGuard{qModuloP};
    auto coefficientCheck = multiplyModuloSecret(result.coefficient, qModuloP, result.prime1);
    const auto coefficientCheckEraseGuard = SecureEraseGuard{coefficientCheck};
    if (!decoder::isOne(coefficientCheck)) {
        decoder::fail("RSA CRT coefficient is not the inverse of prime2 modulo prime1."_el);
    }

    // Transfer the validated normalized components to the caller; its immediately created guard owns erasure next.
    resultEraseGuard.release();
    return result;
}

auto publicKey(const mem::ConstByteSpan privateKey, const mem::ConstByteSpan algorithmIdentifier) -> mem::ByteBlock {
    auto key = decodePrivateKey(privateKey);
    const auto keyEraseGuard = SecureEraseGuard{key};

    // RFC 3279 section 2.3.1 and RFC 4055 section 3.1: preserve the validated PKCS#8 key AlgorithmIdentifier.
    const auto modulusBytes = rsa_signature::numberToBigEndian(key.modulus, key.encodedLength);
    const auto modulusInteger = der_encoder::positiveInteger(modulusBytes.span());
    const auto exponentInteger = der_encoder::positiveInteger(key.publicExponent.span());
    auto rsaChildren = mem::ByteBlockEditor{};
    rsaChildren.append(modulusInteger);
    rsaChildren.append(exponentInteger);
    const auto rsaPublicKey = der_encoder::sequence(rsaChildren.span());
    const auto subjectPublicKey = der_encoder::bitString(rsaPublicKey.span());
    auto spkiChildren = mem::ByteBlockEditor{};
    spkiChildren.append(algorithmIdentifier);
    spkiChildren.append(subjectPublicKey);
    auto spki = der_encoder::sequence(spkiChildren.span());

    // Decode the result through the existing public verifier to enforce RSAE/PSS parameters and public bounds.
    const auto parsedPublicKey = PublicKey::fromDerOrThrow(spki);
    const auto validatedPublicKey = rsa_signature::decodePublicKey(parsedPublicKey);
    if (validatedPublicKey.encodedLength != key.encodedLength || validatedPublicKey.exponent != key.publicExponent) {
        decoder::fail("Derived RSA public key does not preserve its validated components."_el);
    }
    return spki;
}

}
