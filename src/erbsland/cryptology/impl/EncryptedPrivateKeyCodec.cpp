// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncryptedPrivateKeyCodec.hpp"

#include "CryptologyOids.hpp"
#include "DerEncoder.hpp"
#include "DerParser.hpp"
#include "SecureEraseGuard.hpp"

#include "algorithm/HmacSha256.hpp"
#include "symmetric/AesCbcState.hpp"

#include "../asn1/Asn1ObjectIdentifier.hpp"
#include "../asn1/Asn1UniversalType.hpp"
#include "../CryptologyError.hpp"
#include "../symmetric/SymmetricEncryptionType.hpp"

#include "../../core/Application.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology::impl::encrypted_private_key_codec {

using namespace mem;
using namespace text::literals;
using namespace unit;

[[nodiscard]] auto encryptCbc(ConstByteSpan plaintext, ConstByteSpan key, ConstByteSpan iv) -> ByteBlock;
[[nodiscard]] auto decryptCbc(ConstByteSpan ciphertext, ConstByteSpan key, ConstByteSpan iv) -> ByteBlock;

auto encrypt(const ConstByteSpan plaintext, const ConstByteSpan password) -> ByteBlock {
    // RFC 8018 section 6.2.1: choose a salt, derive the encryption key, and encrypt under the selected scheme.
    auto salt = core::application().secureRandom().buildByteBlock(ByteLength{16U});
    const auto saltEraseGuard = SecureEraseGuard{salt};
    auto iv = core::application().secureRandom().buildByteBlock(ByteLength{16U});
    const auto ivEraseGuard = SecureEraseGuard{iv};
    auto key = pbkdf2HmacSha256(password, salt.span(), cIterationCount, 32U);
    const auto keyEraseGuard = SecureEraseGuard{key};
    auto ciphertext = encryptCbc(plaintext, key.span(), iv.span());
    const auto ciphertextEraseGuard = SecureEraseGuard{ciphertext};

    // RFC 8018 appendix A.4: encode PBES2-params directly into the final EncryptedPrivateKeyInfo allocation.
    auto encoder = DerEncoder{};
    const auto root = encoder.beginSequence();
    const auto algorithm = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::pbes2));
    const auto pbes2Parameters = encoder.beginSequence();
    const auto kdf = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::pbkdf2));
    const auto pbkdf2Parameters = encoder.beginSequence();
    encoder.appendOctetString(salt.span());
    encoder.appendPositiveInteger(uint64_t{cIterationCount});
    const auto prf = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::hmacSha256));
    encoder.appendNull();
    encoder.end(prf);
    encoder.end(pbkdf2Parameters);
    encoder.end(kdf);
    const auto cipher = encoder.beginSequence();
    encoder.appendObjectIdentifier(Asn1ObjectIdentifier::fromStringOrThrow(cryptology_oids::aes256Cbc));
    encoder.appendOctetString(iv.span());
    encoder.end(cipher);
    encoder.end(pbes2Parameters);
    encoder.end(algorithm);
    encoder.appendOctetString(ciphertext.span());
    encoder.end(root);
    return encoder.encoded();
}

auto decrypt(const ByteBlock &der, const ConstByteSpan password) -> ByteBlock {
    // RFC 8018 section 6.2.2: accept only the bounded profile written above before deriving any key material.
    const auto root = DerParser{der}.parseDocument();
    if (root.universalType() != Asn1UniversalType::Sequence || root.childCount() != ItemCount{2U}) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto algorithm = root.child(ItemIndex{0U});
    const auto encryptedData = root.child(ItemIndex{1U});
    if (algorithm.universalType() != Asn1UniversalType::Sequence || algorithm.childCount() != ItemCount{2U} ||
        !algorithm.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
        algorithm.child(ItemIndex{0U}).toObjectIdentifier()->toString() != cryptology_oids::pbes2 ||
        encryptedData.universalType() != Asn1UniversalType::OctetString) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto parameters = algorithm.child(ItemIndex{1U});
    if (parameters.universalType() != Asn1UniversalType::Sequence || parameters.childCount() != ItemCount{2U}) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto kdf = parameters.child(ItemIndex{0U});
    const auto cipher = parameters.child(ItemIndex{1U});
    if (kdf.universalType() != Asn1UniversalType::Sequence || kdf.childCount() != ItemCount{2U} ||
        !kdf.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
        kdf.child(ItemIndex{0U}).toObjectIdentifier()->toString() != cryptology_oids::pbkdf2 ||
        cipher.universalType() != Asn1UniversalType::Sequence || cipher.childCount() != ItemCount{2U} ||
        !cipher.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
        cipher.child(ItemIndex{0U}).toObjectIdentifier()->toString() != cryptology_oids::aes256Cbc) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto kdfParameters = kdf.child(ItemIndex{1U});
    if (kdfParameters.universalType() != Asn1UniversalType::Sequence || kdfParameters.childCount() != ItemCount{3U}) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto saltNode = kdfParameters.child(ItemIndex{0U});
    const auto iterationsNode = kdfParameters.child(ItemIndex{1U});
    const auto prf = kdfParameters.child(ItemIndex{2U});
    if (saltNode.universalType() != Asn1UniversalType::OctetString ||
        saltNode.contentData().length() != ByteLength{16U} ||
        iterationsNode.universalType() != Asn1UniversalType::Integer ||
        prf.universalType() != Asn1UniversalType::Sequence || prf.childCount() != ItemCount{2U} ||
        !prf.child(ItemIndex{0U}).toObjectIdentifier().has_value() ||
        prf.child(ItemIndex{0U}).toObjectIdentifier()->toString() != cryptology_oids::hmacSha256 ||
        prf.child(ItemIndex{1U}).universalType() != Asn1UniversalType::Null) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    const auto iterationBytes = iterationsNode.contentData();
    if (iterationBytes.isEmpty() || iterationBytes.length() > ByteLength{4U} ||
        (iterationBytes.span().front().toUInt8() & 0x80U) != 0U) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    auto iterations = uint32_t{};
    for (const auto byte : iterationBytes.span()) {
        iterations = (iterations << 8U) | byte.toUInt32();
    }
    const auto ivNode = cipher.child(ItemIndex{1U});
    const auto ciphertext = encryptedData.contentData();
    if (iterations == 0U || iterations > cMaximumIterationCount ||
        ivNode.universalType() != Asn1UniversalType::OctetString || ivNode.contentData().length() != ByteLength{16U} ||
        ciphertext.isEmpty() || ciphertext.length().toSizeT() % 16U != 0U) {
        throw err::ParseError{"Unsupported encrypted private key."_el};
    }
    auto key = pbkdf2HmacSha256(password, saltNode.contentData().span(), iterations, 32U);
    const auto keyEraseGuard = SecureEraseGuard{key};
    return decryptCbc(ciphertext.span(), key.span(), ivNode.contentData().span());
}

auto encryptCbc(const ConstByteSpan plaintext, const ConstByteSpan key, const ConstByteSpan iv) -> ByteBlock {
    // RFC 5652 section 6.3: always append a complete, unambiguous PKCS#7 padding suffix before CBC encryption.
    auto padded = ByteBlockEditor::fromSpan(plaintext);
    padded.markAsSensitive();
    const auto paddedEraseGuard = SecureEraseGuard{padded};
    const auto paddingLength = 16U - plaintext.size() % 16U;
    padded.append(Byte{static_cast<uint8_t>(paddingLength)}, ByteLength{paddingLength});
    auto state = AesCbcState{SymmetricEncryptionType::Aes256CbcRandomFill, key, iv};
    auto result = state.encrypt(padded.span());
    result.markAsSensitive();
    return result;
}

auto decryptCbc(const ConstByteSpan ciphertext, const ConstByteSpan key, const ConstByteSpan iv) -> ByteBlock {
    auto state = AesCbcState{SymmetricEncryptionType::Aes256CbcRandomFill, key, iv};
    auto plaintext = state.decrypt(ciphertext);
    plaintext.markAsSensitive();
    const auto plaintextEraseGuard = SecureEraseGuard{plaintext};
    [[maybe_unused]] const auto finalBlock = state.finalizeDecryption();
    // RFC 5652 section 6.3: scan the complete final block so padding validation does not reveal the first mismatch.
    const auto paddingLength = plaintext.span().back().toUInt8();
    auto invalid = static_cast<uint8_t>(paddingLength == 0U || paddingLength > 16U);
    for (auto offset = std::size_t{1U}; offset <= 16U; ++offset) {
        const auto checked = static_cast<uint8_t>(offset <= paddingLength);
        const auto mask = static_cast<uint8_t>(uint8_t{} - checked);
        invalid |=
            static_cast<uint8_t>((plaintext.span()[plaintext.span().size() - offset].toUInt8() ^ paddingLength) & mask);
    }
    if (invalid != 0U) {
        throw CryptologyError{"Private-key decryption failed."_el};
    }
    auto result = plaintext.slice(ByteIndex::zero(), ByteLength{plaintext.length().toSizeT() - paddingLength});
    result.markAsSensitive();
    return result;
}

}
