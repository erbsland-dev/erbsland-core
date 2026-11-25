// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "EncryptedPrivateKeyCodec.hpp"

#include "CryptologyAsn1Limits.hpp"
#include "CryptologyOids.hpp"
#include "SecureEraseGuard.hpp"

#include "algorithm/HmacSha256.hpp"
#include "symmetric/AesCbcState.hpp"

#include "../CryptologyError.hpp"
#include "../symmetric/SymmetricEncryptionType.hpp"

#include "../../core/impl/application_data/ApplicationRandomData.hpp"
#include "../../core/impl/ApplicationDataAccess.hpp"
#include "../../data/asn1/Asn1ObjectIdentifier.hpp"
#include "../../data/asn1/Asn1UniversalType.hpp"
#include "../../data/der/DerEncoder.hpp"
#include "../../data/der/DerParser.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/Byte.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteCount.hpp"
#include "../../unit/ByteIndex.hpp"
#include "../../unit/ItemIndex.hpp"

namespace erbsland::cryptology::impl::encrypted_private_key_codec {

using data::asn1::Asn1ObjectIdentifier;
using data::asn1::Asn1UniversalType;

using namespace err;
using namespace mem;
using namespace text::literals;
using namespace unit;

[[nodiscard]] auto encryptCbc(ConstByteSpan plaintext, ConstByteSpan key, ConstByteSpan iv) -> ByteBlock;
[[nodiscard]] auto decryptCbc(ConstByteSpan ciphertext, ConstByteSpan key, ConstByteSpan iv) -> ByteBlock;

auto encrypt(const ConstByteSpan plaintext, const ConstByteSpan password) -> ByteBlock {
    // RFC 8018 section 6.2.1: choose a salt, derive the encryption key, and encrypt under the selected scheme.
    const auto randomData = core::impl::applicationData()->random().get();
    auto salt = randomData->secureRandom().buildByteBlock(ByteCount{16U});
    const auto saltEraseGuard = SecureEraseGuard{salt};
    auto iv = randomData->secureRandom().buildByteBlock(ByteCount{16U});
    const auto ivEraseGuard = SecureEraseGuard{iv};
    auto key = pbkdf2HmacSha256(password, salt.span(), cIterationCount, 32U);
    const auto keyEraseGuard = SecureEraseGuard{key};
    auto ciphertext = encryptCbc(plaintext, key.span(), iv.span());
    const auto ciphertextEraseGuard = SecureEraseGuard{ciphertext};

    // RFC 8018 appendix A.4: encode PBES2-params directly into the final EncryptedPrivateKeyInfo allocation.
    auto encoder = data::der::DerEncoder{};
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
    const auto root = data::der::DerParser::parseOrThrow(der, cCryptologyAsn1Limits);
    if (root.universalType() != Asn1UniversalType::Sequence || root.childCount() != ItemCount{2U}) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto algorithm = root.child(ItemIndex{0U});
    const auto ciphertext = root.child(ItemIndex{1U}).toOctetString();
    const auto algorithmOid = algorithm.child(ItemIndex{0U}).toObjectIdentifier();
    if (algorithm.universalType() != Asn1UniversalType::Sequence || algorithm.childCount() != ItemCount{2U} ||
        !algorithmOid || algorithmOid->toString() != cryptology_oids::pbes2 || !ciphertext) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto parameters = algorithm.child(ItemIndex{1U});
    if (parameters.universalType() != Asn1UniversalType::Sequence || parameters.childCount() != ItemCount{2U}) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto kdf = parameters.child(ItemIndex{0U});
    const auto cipher = parameters.child(ItemIndex{1U});
    const auto kdfOid = kdf.child(ItemIndex{0U}).toObjectIdentifier();
    const auto cipherOid = cipher.child(ItemIndex{0U}).toObjectIdentifier();
    if (kdf.universalType() != Asn1UniversalType::Sequence || kdf.childCount() != ItemCount{2U} || !kdfOid ||
        kdfOid->toString() != cryptology_oids::pbkdf2 || cipher.universalType() != Asn1UniversalType::Sequence ||
        cipher.childCount() != ItemCount{2U} || !cipherOid || cipherOid->toString() != cryptology_oids::aes256Cbc) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto kdfParameters = kdf.child(ItemIndex{1U});
    if (kdfParameters.universalType() != Asn1UniversalType::Sequence || kdfParameters.childCount() != ItemCount{3U}) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto salt = kdfParameters.child(ItemIndex{0U}).toOctetString();
    const auto iterations = kdfParameters.child(ItemIndex{1U}).toUnsignedInteger();
    const auto prf = kdfParameters.child(ItemIndex{2U});
    const auto prfOid = prf.child(ItemIndex{0U}).toObjectIdentifier();
    if (!salt || salt->length() != ByteCount{16U} || prf.universalType() != Asn1UniversalType::Sequence ||
        prf.childCount() != ItemCount{2U} || !prfOid || prfOid->toString() != cryptology_oids::hmacSha256 ||
        !prf.child(ItemIndex{1U}).isNull()) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    const auto iv = cipher.child(ItemIndex{1U}).toOctetString();
    if (!iterations || *iterations == 0U || *iterations > cMaximumIterationCount || !iv ||
        iv->length() != ByteCount{16U} || ciphertext->isEmpty() || ciphertext->length().toSizeT() % 16U != 0U) {
        throw ParseError{"Unsupported encrypted private key."_el};
    }
    auto key = pbkdf2HmacSha256(password, salt->span(), static_cast<uint32_t>(*iterations), 32U);
    const auto keyEraseGuard = SecureEraseGuard{key};
    return decryptCbc(ciphertext->span(), key.span(), iv->span());
}

auto encryptCbc(const ConstByteSpan plaintext, const ConstByteSpan key, const ConstByteSpan iv) -> ByteBlock {
    // RFC 5652 section 6.3: always append a complete, unambiguous PKCS#7 padding suffix before CBC encryption.
    auto padded = ByteBlockEditor::fromSpan(plaintext);
    padded.markAsSensitive();
    const auto paddedEraseGuard = SecureEraseGuard{padded};
    const auto paddingLength = 16U - plaintext.size() % 16U;
    padded.append(Byte{static_cast<uint8_t>(paddingLength)}, ByteCount{paddingLength, ByteCount::Unchecked{}});
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
    auto result =
        plaintext.slice(ByteIndex::zero(), plaintext.length() - ByteCount{paddingLength, ByteCount::Unchecked{}});
    result.markAsSensitive();
    return result;
}

}
