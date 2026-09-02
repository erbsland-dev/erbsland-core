// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashData.hpp"

#include "algorithm/HmacSha256.hpp"

#include "../unsafe/UnsafeCustomPasswordHashParameters.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParseError.hpp"
#include "../../mem/ByteArray.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/base_n/BaseNDecoder.hpp"
#include "../../text/base_n/BaseNEncoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/impl/NamedKeyEntry.hpp"
#include "../../text/impl/NamedKeyFormat.hpp"
#include "../../text/impl/NamedKeyParser.hpp"
#include "../../text/impl/UnsafeU8StringAccess.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"
#include "../../util/List.hpp"

#include <limits>
#include <utility>

namespace erbsland::cryptology::impl {

using namespace text;
using namespace literals;
using namespace mem;
using namespace unit;
using text::impl::NamedKeyEntry;
using text::impl::NamedKeyFormat;
using text::impl::NamedKeyParser;

PasswordHashData::PasswordHashData(
    PasswordHashPolicy policy,
    const bool keyed,
    std::optional<String> keyIdentifier,
    ByteBlock salt,
    ByteBlock verifier,
    String headerThroughSalt,
    String canonical) noexcept :
    _policy{std::move(policy)},
    _keyed{keyed},
    _keyIdentifier{std::move(keyIdentifier)},
    _salt{std::move(salt)},
    _verifier{std::move(verifier)},
    _headerThroughSalt{std::move(headerThroughSalt)},
    _canonical{std::move(canonical)} {
}

auto PasswordHashData::create(
    PasswordHashPolicy policy, const PasswordHashKey *key, const ConstByteSpan salt, const ConstByteSpan rawVerifier)
    -> PasswordHashDataPtr {
    if (salt.size() != policy.saltLength().toSizeT() || rawVerifier.size() != policy.outputLength().toSizeT()) {
        throw err::LogicError{"Password hash data violates the policy length invariant"};
    }
    const auto keyed = key != nullptr;
    const auto keyIdentifier = keyed ? key->_identifier : std::optional<String>{};
    auto saltData = ByteBlock::fromSpan(salt);
    auto header = buildHeader(policy, keyed, keyIdentifier, saltData.span());
    auto verifier = protectVerifier(rawVerifier, header, key);
    auto canonical = buildRecord(header, verifier.span());
    return PasswordHashDataPtr{new PasswordHashData{
        std::move(policy),
        keyed,
        keyIdentifier,
        std::move(saltData),
        std::move(verifier),
        std::move(header),
        std::move(canonical),
    }};
}

auto PasswordHashData::fromStringOrThrow(const String &text) -> PasswordHashDataPtr {
    if (text.isEmpty() || text.length() > ByteLength{512U}) {
        throw err::ParseError{"Password hash record is empty or exceeds 512 bytes"};
    }
    static const auto cVisibleAscii = CharSet::fromRange(Char{U'!'}, Char{U'~'});
    if (!text.containsOnly(cVisibleAscii)) {
        throw err::ParseError{"Password hash record must contain visible ASCII only"};
    }

    auto parserReader = StringCharReader{text};
    const auto fields = NamedKeyParser{parserReader, passwordHashFormat()}.readAllEntries();
    if (fields.count().toSizeT() < 9U || fieldValue(fields, ItemIndex{0U}, Field::Format) != "el-password-hash"_el ||
        fieldValue(fields, ItemIndex{1U}, Field::Version) != "1"_el) {
        throw err::ParseError{"Unsupported password hash format or version"};
    }

    const auto algorithmText = fieldValue(fields, ItemIndex{2U}, Field::Algorithm);
    auto policy = PasswordHashPolicy{};
    auto modeIndex = ItemIndex::zero();
    if (algorithmText == "argon2id"_el) {
        if (fields.count().toSizeT() != 10U && fields.count().toSizeT() != 11U) {
            throw err::ParseError{"Unexpected Argon2id password hash fields"};
        }
        if (fieldValue(fields, ItemIndex{3U}, Field::ArgonVersion) != "19"_el) {
            throw err::ParseError{"Unsupported Argon2 version"};
        }
        const auto memory = static_cast<uint32_t>(parseCanonicalInteger(
            fieldValue(fields, ItemIndex{4U}, Field::Memory), std::numeric_limits<uint32_t>::max()));
        const auto passes = static_cast<uint32_t>(parseCanonicalInteger(
            fieldValue(fields, ItemIndex{5U}, Field::Passes), std::numeric_limits<uint32_t>::max()));
        const auto lanes = static_cast<uint32_t>(parseCanonicalInteger(
            fieldValue(fields, ItemIndex{6U}, Field::Parallelization), std::numeric_limits<uint32_t>::max()));
        if (!unsafe::UnsafeCustomPasswordHashParameters::areValidArgon2idCosts(memory, passes, lanes)) {
            throw err::ParseError{"Argon2id costs exceed the supported safety bounds"};
        }
        policy = PasswordHashPolicy{unsafe::UnsafeCustomPasswordHashParameters::argon2id(memory, passes, lanes)};
        modeIndex = ItemIndex{7U};
    } else if (algorithmText == "scrypt"_el) {
        if (fields.count().toSizeT() != 9U && fields.count().toSizeT() != 10U) {
            throw err::ParseError{"Unexpected scrypt password hash fields"};
        }
        const auto cost =
            parseCanonicalInteger(fieldValue(fields, ItemIndex{3U}, Field::Cost), std::numeric_limits<uint64_t>::max());
        const auto blockSize = static_cast<uint32_t>(parseCanonicalInteger(
            fieldValue(fields, ItemIndex{4U}, Field::BlockSize), std::numeric_limits<uint32_t>::max()));
        const auto parallelization = static_cast<uint32_t>(parseCanonicalInteger(
            fieldValue(fields, ItemIndex{5U}, Field::Parallelization), std::numeric_limits<uint32_t>::max()));
        if (!unsafe::UnsafeCustomPasswordHashParameters::areValidScryptCosts(cost, blockSize, parallelization)) {
            throw err::ParseError{"scrypt costs exceed the supported safety bounds"};
        }
        policy =
            PasswordHashPolicy{unsafe::UnsafeCustomPasswordHashParameters::scrypt(cost, blockSize, parallelization)};
        modeIndex = ItemIndex{6U};
    } else {
        throw err::ParseError{"Unsupported password hashing algorithm"};
    }

    const auto mode = fieldValue(fields, modeIndex, Field::Mode);
    const auto keyed = mode == "hmac-sha256"_el;
    if (!keyed && mode != "none"_el) {
        throw err::ParseError{"Unsupported password hash protection mode"};
    }
    auto index = modeIndex + ItemCount{1U};
    auto keyIdentifier = std::optional<String>{};
    const auto hasKeyIdentifier = index.isWithin(fields.count()) &&
        fields.getRefOrThrow(index).keyIndex() == static_cast<int>(Field::KeyIdentifier);
    if (keyed && hasKeyIdentifier) {
        const auto identifier = fieldValue(fields, index++, Field::KeyIdentifier);
        if (!PasswordHashKey::isValidIdentifier(identifier)) {
            throw err::ParseError{"Invalid password hash key identifier"};
        }
        keyIdentifier = identifier;
    }
    if (!keyed && hasKeyIdentifier) {
        throw err::ParseError{"Unkeyed password hashes cannot contain a key identifier"};
    }

    const auto saltText = fieldValue(fields, index++, Field::Salt);
    const auto verifierText = fieldValue(fields, index++, Field::Data);
    if (index != ItemIndex::end(fields.count())) {
        throw err::ParseError{"Password hash contains unknown fields"};
    }
    auto salt = decodeBytes(saltText, policy.saltLength());
    auto verifier = decodeBytes(verifierText, policy.outputLength());
    auto header = buildHeader(policy, keyed, keyIdentifier, salt.span());
    auto canonical = buildRecord(header, verifier.span());
    if (canonical != text) {
        throw err::ParseError{"Password hash record is not canonical"};
    }

    return PasswordHashDataPtr{new PasswordHashData{
        std::move(policy),
        keyed,
        std::move(keyIdentifier),
        std::move(salt),
        std::move(verifier),
        std::move(header),
        std::move(canonical),
    }};
}

auto PasswordHashData::matchesVerifier(const ConstByteSpan rawVerifier, const PasswordHashKey *key) const -> bool {
    if (_keyed != (key != nullptr) || (key != nullptr && key->_identifier != _keyIdentifier)) {
        return false;
    }
    const auto candidate = protectVerifier(rawVerifier, _headerThroughSalt, key);
    return _verifier.isEqualConstTime(candidate);
}

void PasswordHashData::performDummyVerification(
    const PasswordHashPolicy &policy,
    const PasswordHashKey *key,
    const ConstByteSpan salt,
    const ConstByteSpan rawVerifier) {
    if (salt.size() != policy.saltLength().toSizeT() || rawVerifier.size() != policy.outputLength().toSizeT()) {
        throw err::LogicError{"Dummy password verification violates the policy length invariant"};
    }
    const auto keyIdentifier = key != nullptr ? key->_identifier : std::optional<String>{};
    const auto header = buildHeader(policy, key != nullptr, keyIdentifier, salt);
    const auto candidate = protectVerifier(rawVerifier, header, key);
    auto dummyVerifier = ByteBlockEditor{candidate.length()};
    dummyVerifier.markAsSensitive();
    if (candidate.isEqualConstTime(dummyVerifier.span())) {
        // Keep the comparison result observable through the secure-erased dummy storage.
        dummyVerifier.fill(Byte{1U});
    }
}

auto PasswordHashData::needsReplacement(
    const PasswordHashPolicy &policy, const PasswordHashKey *activeKey) const noexcept -> bool {
    if (!policy.isEqualTo(_policy) || _keyed != (activeKey != nullptr)) {
        return true;
    }
    return activeKey != nullptr && activeKey->_identifier != _keyIdentifier;
}

auto PasswordHashData::storageBase64Format() -> base_n::BaseNFormat {
    auto format = base_n::BaseNFormat::base64Url();
    format.clearFlags(base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(CharSet{});
    return format;
}

auto PasswordHashData::passwordHashFormat() -> const NamedKeyFormat & {
    static const auto keys = NamedKeyFormat::Keys{{
        {"f"_el, static_cast<int>(Field::Format)},
        {"v"_el, static_cast<int>(Field::Version)},
        {"a"_el, static_cast<int>(Field::Algorithm)},
        {"av"_el, static_cast<int>(Field::ArgonVersion)},
        {"m"_el, static_cast<int>(Field::Memory)},
        {"t"_el, static_cast<int>(Field::Passes)},
        {"p"_el, static_cast<int>(Field::Parallelization)},
        {"n"_el, static_cast<int>(Field::Cost)},
        {"r"_el, static_cast<int>(Field::BlockSize)},
        {"x"_el, static_cast<int>(Field::Mode)},
        {"i"_el, static_cast<int>(Field::KeyIdentifier)},
        {"s"_el, static_cast<int>(Field::Salt)},
        {"d"_el, static_cast<int>(Field::Data)},
    }};
    static const auto format =
        NamedKeyFormat{}.setKeys(keys).setValueSeparator(U':').setKeysWithoutValuesAllowed(false).setValueListAllowed(
            false);
    return format;
}

auto PasswordHashData::fieldValue(const FieldList &fields, const ItemIndex index, const Field expected)
    -> const String & {
    if (!index.isWithin(fields.count())) {
        throw err::ParseError{"Password hash fields are missing or out of order"};
    }
    const auto &entry = fields.getRefOrThrow(index);
    if (entry.keyIndex() != static_cast<int>(expected)) {
        throw err::ParseError{"Password hash fields are missing or out of order"};
    }
    return entry.value();
}

auto PasswordHashData::parseCanonicalInteger(const String &text, const uint64_t maximum) -> uint64_t {
    if (text.isEmpty() || (text.characterLength() > CpLength::one() && text.charAt(StringSide::Front) == U'0')) {
        throw err::ParseError{"Password hash contains a noncanonical integer"};
    }
    auto options = IntegerParseOptions::parserDefault();
    options.setFixedBase(IntegerBase::Decimal);
    const auto result = text.toIntegerOrThrow<uint64_t>(options);
    if (result > maximum) {
        throw err::ParseError{"Password hash contains an invalid integer"};
    }
    return result;
}

auto PasswordHashData::encodeBytes(const ConstByteSpan bytes) -> String {
    return base_n::BaseNEncoder{ByteBlock::fromSpan(bytes), storageBase64Format()}.toString();
}

auto PasswordHashData::decodeBytes(const String &encoded, const ByteLength expectedLength) -> ByteBlock {
    const auto expectedEncodedLength = ByteLength::fromSizeT((expectedLength.toSizeT() * 8U + 5U) / 6U);
    if (encoded.length() != expectedEncodedLength) {
        throw err::ParseError{"Password hash contains data with an invalid length"};
    }
    const auto result = base_n::BaseNDecoder{encoded, storageBase64Format()}.toDataOrThrow(expectedLength);
    if (result.length() != expectedLength || encodeBytes(result.span()) != encoded) {
        throw err::ParseError{"Password hash contains noncanonical Base64url data"};
    }
    return result;
}

void PasswordHashData::appendField(StringEditor &result, const StringLiteral &key, const String &value) {
    result.append(U',').append(key).append(U':').append(value);
}

auto PasswordHashData::buildHeader(
    const PasswordHashPolicy &policy,
    const bool keyed,
    const std::optional<String> &keyIdentifier,
    const ConstByteSpan salt) -> String {
    auto result = StringEditor{"f:el-password-hash,v:1,a:"_el};
    result.append(policy.algorithm().toString());
    if (policy.algorithm() == PasswordHashAlgorithm::Argon2id) {
        appendField(result, "av"_el, "19"_el);
        appendField(result, "m"_el, String::fromInteger(policy.memoryKiB()));
        appendField(result, "t"_el, String::fromInteger(policy.passes()));
        appendField(result, "p"_el, String::fromInteger(policy.lanes()));
    } else {
        appendField(result, "n"_el, String::fromInteger(policy.scryptCost()));
        appendField(result, "r"_el, String::fromInteger(policy.scryptBlockSize()));
        appendField(result, "p"_el, String::fromInteger(policy.scryptParallelization()));
    }
    appendField(result, "x"_el, keyed ? "hmac-sha256"_el : "none"_el);
    if (keyIdentifier.has_value()) {
        appendField(result, "i"_el, *keyIdentifier);
    }
    appendField(result, "s"_el, encodeBytes(salt));
    return String{result};
}

auto PasswordHashData::buildRecord(const String &header, const ConstByteSpan verifier) -> String {
    auto result = StringEditor{header};
    appendField(result, "d"_el, encodeBytes(verifier));
    return String{result};
}

auto PasswordHashData::protectVerifier(
    const ConstByteSpan rawVerifier, const String &header, const PasswordHashKey *key) -> ByteBlock {
    if (key == nullptr) {
        return ByteBlock::fromSpan(rawVerifier);
    }
    const auto headerBytes = toConstByteSpan(text::impl::UnsafeU8StringAccess{header}.dataSpan());
    const auto separator = ByteArray<1>{Byte{0U}};
    return ByteBlock{hmacSha256(
        key->_key.span(),
        {
            headerBytes,
            separator.span(),
            rawVerifier,
        })};
}

}
