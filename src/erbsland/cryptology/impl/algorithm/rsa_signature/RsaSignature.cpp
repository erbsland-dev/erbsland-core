// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "RsaSignature.hpp"

#include "../../../../mem/Byte.hpp"
#include "../../../../mem/ByteBlockEditor.hpp"
#include "../../../../mem/Endianness.hpp"
#include "../../../../text/Literals.hpp"
#include "../../../../unit/ByteIndex.hpp"
#include "../../../../unit/ByteLength.hpp"
#include "../../../Hasher.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace erbsland::cryptology::impl::rsa_signature {

using namespace text::literals;
using namespace unit;

auto verify(
    const PublicKey &publicKey,
    const X509AlgorithmIdentifier &signatureAlgorithm,
    const mem::ConstByteSpan message,
    const mem::ConstByteSpan signature) -> bool {
    // RFC 5280 sections 4.1.1.2 and 4.1.2.7 bind a signature AlgorithmIdentifier to the issuer's public-key info.
    const auto parameters = decodeSignatureParameters(signatureAlgorithm);
    const auto key = decodePublicKey(publicKey);
    if (!parametersMatchKey(parameters, key.restrictions)) {
        throwParseError("The RSA signature algorithm violates the public-key algorithm restrictions."_el);
    }

    // RFC 8017 sections 8.1.2 and 8.2.2 first apply RSAVP1 and then verify the selected encoded-message format.
    const auto encodedMessage = rsaVerificationPrimitive(key, signature);
    if (!encodedMessage.has_value()) {
        return false;
    }
    if (parameters.padding == Padding::Pss) {
        return verifyPss(key, parameters, message, *encodedMessage);
    }
    return verifyPkcs1V15(key, parameters, message, *encodedMessage);
}

auto verifyPss(
    const PublicKeyData &key,
    const SignatureParameters &parameters,
    const mem::ConstByteSpan message,
    const mem::ByteBlock &encodedMessage) -> bool {
    // RFC 8017 section 8.1.2 step 2 sets `emBits = modBits - 1`; I2OSP may therefore contribute one leading octet.
    const auto emBits = key.modulusBits - 1U;
    const auto emLength = (emBits + 7U) / 8U;
    if (emLength > encodedMessage.span().size()) {
        return false;
    }
    const auto leadingLength = encodedMessage.span().size() - emLength;
    for (auto index = std::size_t{}; index < leadingLength; ++index) {
        if (encodedMessage.span()[index].toUInt8() != 0U) {
            return false;
        }
    }
    const auto encoded = encodedMessage.span().subspan(leadingLength);
    const auto hashLength = parameters.hash.digestSize().toSizeT();

    // RFC 8017 section 9.1.2 steps 1--3 hash M and enforce the minimum encoded length.
    const auto messageHash = hashMessage(parameters.hash, message);
    if (emLength < hashLength + parameters.saltLength + 2U) {
        return false;
    }

    // RFC 8017 section 9.1.2 step 4 requires the fixed trailer octet 0xbc.
    if (encoded.back().toUInt8() != 0xbcU) {
        return false;
    }
    const auto maskedDbLength = emLength - hashLength - 1U;
    const auto maskedDb = encoded.first(maskedDbLength);
    const auto encodedHash = encoded.subspan(maskedDbLength, hashLength);

    // RFC 8017 section 9.1.2 step 6 requires the unused high bits of maskedDB to be zero.
    const auto unusedHighBits = 8U * emLength - emBits;
    const auto forbiddenHighBits = static_cast<uint8_t>(0xffU << (8U - unusedHighBits));
    if ((maskedDb.front().toUInt8() & forbiddenHighBits) != 0U) {
        return false;
    }

    // RFC 8017 section 9.1.2 steps 7--9 compute dbMask = MGF(H), recover DB, and clear its unused high bits.
    const auto dbMask = mgf1(parameters.hash, encodedHash, maskedDbLength);
    auto db = mem::ByteBlockEditor::fromSpan(maskedDb);
    db.xorWithOrThrow(dbMask);
    db.set(
        ByteIndex::zero(),
        mem::Byte{static_cast<uint8_t>(db.get(ByteIndex::zero()).toUInt8() & (0xffU >> unusedHighBits))});

    // RFC 8017 section 9.1.2 step 10 requires DB = PS || 0x01 || salt with an all-zero PS.
    const auto psLength = emLength - hashLength - parameters.saltLength - 2U;
    for (auto index = std::size_t{}; index < psLength; ++index) {
        if (db.get(ByteIndex{index}).toUInt8() != 0U) {
            return false;
        }
    }
    if (db.get(ByteIndex{psLength}).toUInt8() != 1U) {
        return false;
    }
    const auto salt = db.slice(ByteIndex{psLength + 1U}, ByteLength::fromSizeT(parameters.saltLength));

    // RFC 8017 section 9.1.2 steps 11--13 compute H' = Hash(0x00^8 || mHash || salt) and compare it with H.
    auto hashInput = mem::ByteBlockEditor{ByteLength{8U}};
    hashInput.append(messageHash);
    hashInput.append(salt);
    const auto expectedHash =
        hashMessage(parameters.hash, hashInput.slice(ByteIndex::zero(), hashInput.length()).span());
    return expectedHash.isEqualConstTime(encodedHash);
}

auto verifyPkcs1V15(
    [[maybe_unused]] const PublicKeyData &key,
    const SignatureParameters &parameters,
    const mem::ConstByteSpan message,
    const mem::ByteBlock &encodedMessage) -> bool {
    // RFC 8017 section 9.2 steps 1--2 hash M and encode the digest in the algorithm-specific DER DigestInfo T.
    const auto digest = hashMessage(parameters.hash, message);
    // RFC 8017 section 9.2 note 1 gives the exact DER DigestInfo prefixes for SHA-256 and SHA-384.
    static constexpr auto cSha256DigestInfo = std::array<uint8_t, 19U>{
        0x30U,
        0x31U,
        0x30U,
        0x0dU,
        0x06U,
        0x09U,
        0x60U,
        0x86U,
        0x48U,
        0x01U,
        0x65U,
        0x03U,
        0x04U,
        0x02U,
        0x01U,
        0x05U,
        0x00U,
        0x04U,
        0x20U};
    static constexpr auto cSha384DigestInfo = std::array<uint8_t, 19U>{
        0x30U,
        0x41U,
        0x30U,
        0x0dU,
        0x06U,
        0x09U,
        0x60U,
        0x86U,
        0x48U,
        0x01U,
        0x65U,
        0x03U,
        0x04U,
        0x02U,
        0x02U,
        0x05U,
        0x00U,
        0x04U,
        0x30U};
    const auto digestInfo = parameters.hash == HashAlgorithm::Sha2_256
        ? mem::toConstByteSpan(std::span<const uint8_t>{cSha256DigestInfo})
        : mem::toConstByteSpan(std::span<const uint8_t>{cSha384DigestInfo});
    const auto encodedLength = encodedMessage.span().size();
    const auto trailerLength = digestInfo.size() + digest.span().size();
    if (encodedLength < trailerLength + 11U) {
        return false;
    }

    // RFC 8017 section 9.2 steps 4--5 construct EM = 0x00 || 0x01 || PS || 0x00 || T with at least eight 0xff bytes.
    const auto paddingLength = encodedLength - trailerLength - 3U;
    auto expected = mem::ByteBlockEditor{};
    expected.reserve(ByteLength::fromSizeT(encodedLength));
    expected.append(mem::Byte{0x00U});
    expected.append(mem::Byte{0x01U});
    expected.append(mem::Byte{0xffU}, ByteLength::fromSizeT(paddingLength));
    expected.append(mem::Byte{0x00U});
    expected.append(digestInfo);
    expected.append(digest);

    // RFC 8017 section 8.2.2 step 4 requires exact equality; no BER or partial DigestInfo acceptance is allowed.
    return expected.isEqualConstTime(encodedMessage.span());
}

auto mgf1(const HashAlgorithm hash, const mem::ConstByteSpan seed, const std::size_t length) -> mem::ByteBlock {
    auto mask = mem::ByteBlockEditor{};
    mask.reserve(ByteLength::fromSizeT(length));
    // RFC 8017 appendix B.2.1 steps 1--3 concatenate Hash(seed || I2OSP(counter, 4)) blocks.
    for (auto counter = uint32_t{}; mask.length().toSizeT() < length; ++counter) {
        auto hasher = Hasher{hash};
        hasher.update(seed);
        auto counterBytes = mem::ByteBlockEditor{};
        counterBytes.appendInteger(counter, mem::Endianness::Big);
        hasher.update(counterBytes.slice(ByteIndex::zero(), counterBytes.length()));
        mask.append(hasher.finalize());
    }
    mask.resize(ByteLength::fromSizeT(length));
    return mem::ByteBlock{mask};
}

auto hashMessage(const HashAlgorithm hash, const mem::ConstByteSpan message) -> mem::ByteBlock {
    auto hasher = Hasher{hash};
    hasher.update(message);
    return hasher.finalize();
}

auto parametersMatchKey(
    const SignatureParameters &signature, const std::optional<SignatureParameters> &restrictions) noexcept -> bool {
    if (!restrictions.has_value()) {
        return true;
    }
    // RFC 4055 section 3.3: a PSS-restricted key requires matching hash, MGF, and trailer; signature salt may be
    // longer.
    return signature.padding == Padding::Pss && signature.hash == restrictions->hash &&
        signature.saltLength >= restrictions->saltLength;
}

}
