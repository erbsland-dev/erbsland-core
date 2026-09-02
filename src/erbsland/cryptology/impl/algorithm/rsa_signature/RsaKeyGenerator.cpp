// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaKeyGenerator.hpp"

#include "Number.hpp"

#include "../../../../err/ParameterError.hpp"
#include "../../../../mem/ByteBlock.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../CryptologyError.hpp"
#include "../../DerEncoder.hpp"
#include "../../SecureEraseGuard.hpp"

namespace erbsland::cryptology::impl::rsa_key_generator {

using namespace mem;
using namespace text::literals;
using rsa_signature::Number;

constexpr auto cPublicExponent = uint32_t{65537U};

auto generate(const std::size_t modulusBits) -> ByteBlock {
    if (modulusBits != 2048U && modulusBits != 3072U && modulusBits != 4096U) {
        throw err::ParameterError{"RSA generation supports only 2048, 3072, or 4096 bits."_el, "modulusBits"_el};
    }
    const auto primeBits = modulusBits / 2U;
    const auto modulusWords = modulusBits / rsa_signature::cWordBits;
    auto p = Number::generatePrime(primeBits);
    const auto pEraseGuard = SecureEraseGuard{p};
    auto q = Number{};
    const auto qEraseGuard = SecureEraseGuard{q};
    do {
        q.secureErase();
        q = Number::generatePrime(primeBits);
    } while (!p.isSeparatedFrom(q, primeBits));

    auto modulus = p.multiplied(q, modulusWords);
    const auto modulusEraseGuard = SecureEraseGuard{modulus};
    if (modulus.bitLength() != modulusBits) {
        throw CryptologyError{"RSA prime generation did not produce the requested modulus width."_el};
    }
    auto pMinusOne = p.subtractOne();
    const auto pMinusOneEraseGuard = SecureEraseGuard{pMinusOne};
    auto qMinusOne = q.subtractOne();
    const auto qMinusOneEraseGuard = SecureEraseGuard{qMinusOne};
    auto phi = pMinusOne.multiplied(qMinusOne, modulusWords);
    const auto phiEraseGuard = SecureEraseGuard{phi};
    auto d = phi.inversePublicExponent();
    const auto dEraseGuard = SecureEraseGuard{d};
    if (d.bitLength() <= modulusBits / 2U) {
        throw CryptologyError{"Generated RSA private exponent is below the fixed safety bound."_el};
    }
    auto dP = pMinusOne.inversePublicExponent();
    const auto dPEraseGuard = SecureEraseGuard{dP};
    auto dQ = qMinusOne.inversePublicExponent();
    const auto dQEraseGuard = SecureEraseGuard{dQ};
    auto qModuloP = q.reducedSecret(p);
    const auto qModuloPEraseGuard = SecureEraseGuard{qModuloP};
    auto pMinusTwo = pMinusOne.subtractOne();
    const auto pMinusTwoEraseGuard = SecureEraseGuard{pMinusTwo};
    auto qInv = qModuloP.poweredModuloSecret(pMinusTwo, p);
    const auto qInvEraseGuard = SecureEraseGuard{qInv};

    auto encoder = DerEncoder{};
    encoder.markAsSensitive();
    const auto root = encoder.beginSequence();
    encoder.appendPositiveInteger(uint64_t{});
    encoder.appendPositiveInteger(modulus.toBigEndian(modulusBits / 8U).span());
    encoder.appendPositiveInteger(uint64_t{cPublicExponent});
    encoder.appendPositiveInteger(d.toBigEndian(modulusBits / 8U).span());
    encoder.appendPositiveInteger(p.toBigEndian(primeBits / 8U).span());
    encoder.appendPositiveInteger(q.toBigEndian(primeBits / 8U).span());
    encoder.appendPositiveInteger(dP.toBigEndian(primeBits / 8U).span());
    encoder.appendPositiveInteger(dQ.toBigEndian(primeBits / 8U).span());
    encoder.appendPositiveInteger(qInv.toBigEndian(primeBits / 8U).span());
    encoder.end(root);
    return encoder.encoded();
}

}
