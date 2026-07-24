// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PasswordHashFormat.hpp"

#include "PasswordHashData.hpp"

#include "../unsafe/UnsafeCustomPasswordHashParameters.hpp"

#include "../../err/ParseError.hpp"
#include "../../text/base_n/BaseNDecoder.hpp"
#include "../../text/base_n/BaseNEncoder.hpp"
#include "../../text/base_n/BaseNFormat.hpp"
#include "../../text/CharSet.hpp"
#include "../../text/impl/NamedKeyFormat.hpp"
#include "../../text/impl/NamedKeyParser.hpp"
#include "../../text/IntegerParseOptions.hpp"
#include "../../text/Literals.hpp"
#include "../../text/StringCharReader.hpp"
#include "../../text/StringEditor.hpp"

#include <cstdint>

namespace erbsland::cryptology::impl {

using namespace text::literals;

namespace {

enum class Field : int {
    Format,
    Version,
    Algorithm,
    ArgonVersion,
    Memory,
    Passes,
    Parallelization,
    Cost,
    BlockSize,
    Mode,
    KeyIdentifier,
    Salt,
    Data,
};

auto storageBase64Format() -> text::base_n::BaseNFormat {
    auto format = text::base_n::BaseNFormat::base64Url();
    format.clearFlags(text::base_n::BaseNFormatFlag::All);
    format.setPadding(std::nullopt);
    format.setWhitespace(text::CharSet{});
    return format;
}

template <typename T>
auto parseCanonicalInteger(const text::String &text) -> T {
    if (text.isEmpty() ||
        (text.characterLength() > unit::CpLength::one() && text.charAt(text::StringSide::Front) == U'0')) {
        throw err::ParseError{"Password hash contains a noncanonical integer"};
    }
    try {
        auto options = text::IntegerParseOptions::parserDefault();
        options.setFixedBase(text::IntegerBase::Decimal);
        return text.toIntegerOrThrow<T>(options);
    } catch (...) {
        throw err::ParseError{"Password hash contains an invalid integer"};
    }
}

auto passwordHashFormat() -> const text::impl::NamedKeyFormat & {
    static const auto keys = text::impl::NamedKeyFormat::Keys{{
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
    static const auto format = text::impl::NamedKeyFormat{}
                                   .setKeys(keys)
                                   .setValueSeparator(U':')
                                   .setKeysWithoutValuesAllowed(false)
                                   .setValueListAllowed(false);
    return format;
}

auto fieldValue(const util::List<text::impl::NamedKeyEntry> &fields, const std::size_t index, const Field expected)
    -> const text::String & {
    const auto &raw = fields.toRawValue();
    if (index >= raw.size() || raw[index].keyIndex() != static_cast<int>(expected)) {
        throw err::ParseError{"Password hash fields are missing or out of order"};
    }
    return raw[index].value();
}

auto validKeyIdentifier(const text::String &identifier) noexcept -> bool {
    if (identifier.isEmpty() || identifier.characterLength() > unit::CpLength{32U}) {
        return false;
    }
    auto reader = text::StringCharReader{identifier};
    while (!reader.isAtEnd()) {
        const auto character = reader.read();
        if (!character.isAsciiAlphanumeric() && character != U'.' && character != U'_' && character != U'-') {
            return false;
        }
    }
    return true;
}

void appendField(text::StringEditor &result, const text::StringLiteral &key, const text::String &value) {
    result.append(U',').append(key).append(U':').append(value);
}

}

auto encodePasswordHashBytes(const mem::ConstByteSpan bytes) -> text::String {
    return text::base_n::BaseNEncoder{mem::ByteBlock::fromSpan(bytes), storageBase64Format()}.toString();
}

auto decodePasswordHashBytes(const text::String &encoded, const unit::ByteLength expectedLength) -> mem::ByteBlock {
    const auto result = text::base_n::BaseNDecoder{encoded, storageBase64Format()}.toDataOrThrow(expectedLength);
    if (result.length() != expectedLength || encodePasswordHashBytes(result.span()) != encoded) {
        throw err::ParseError{"Password hash contains noncanonical Base64url data"};
    }
    return result;
}

auto buildPasswordHashHeader(
    const PasswordHashPolicy &policy,
    const bool keyed,
    const std::optional<text::String> &keyIdentifier,
    const mem::ConstByteSpan salt) -> text::String {
    auto result = text::StringEditor{"f:el-password-hash,v:1,a:"_el};
    result.append(policy.algorithm().toString());
    if (policy.algorithm() == PasswordHashAlgorithm::Argon2id) {
        appendField(result, "av"_el, "19"_el);
        appendField(result, "m"_el, text::String::fromInteger(policy.memoryKiB()));
        appendField(result, "t"_el, text::String::fromInteger(policy.passes()));
        appendField(result, "p"_el, text::String::fromInteger(policy.lanes()));
    } else {
        appendField(result, "n"_el, text::String::fromInteger(policy.scryptCost()));
        appendField(result, "r"_el, text::String::fromInteger(policy.scryptBlockSize()));
        appendField(result, "p"_el, text::String::fromInteger(policy.scryptParallelization()));
    }
    appendField(result, "x"_el, keyed ? "hmac-sha256"_el : "none"_el);
    if (keyIdentifier.has_value()) {
        appendField(result, "i"_el, *keyIdentifier);
    }
    appendField(result, "s"_el, encodePasswordHashBytes(salt));
    return text::String{result};
}

auto buildPasswordHashRecord(const text::String &header, const mem::ConstByteSpan verifier) -> text::String {
    auto result = text::StringEditor{header};
    appendField(result, "d"_el, encodePasswordHashBytes(verifier));
    return text::String{result};
}

auto parsePasswordHash(const text::String &text) -> PasswordHashDataPtr {
    try {
        if (text.isEmpty() || text.length() > unit::ByteLength{512U}) {
            throw err::ParseError{"Password hash record is empty or exceeds 512 bytes"};
        }
        auto recordReader = text::StringCharReader{text};
        while (!recordReader.isAtEnd()) {
            const auto character = recordReader.read();
            if (character < U'!' || character > U'~') {
                throw err::ParseError{"Password hash record must contain visible ASCII only"};
            }
        }

        auto parserReader = text::StringCharReader{text};
        const auto fields = text::impl::NamedKeyParser{parserReader, passwordHashFormat()}.readAllEntries();
        if (fields.count().toSizeT() < 9U || fieldValue(fields, 0U, Field::Format) != "el-password-hash"_el ||
            fieldValue(fields, 1U, Field::Version) != "1"_el) {
            throw err::ParseError{"Unsupported password hash format or version"};
        }
        const auto algorithmText = fieldValue(fields, 2U, Field::Algorithm);
        auto policy = PasswordHashPolicy{};
        auto modeIndex = std::size_t{};
        if (algorithmText == "argon2id"_el) {
            if (fields.count().toSizeT() != 10U && fields.count().toSizeT() != 11U) {
                throw err::ParseError{"Unexpected Argon2id password hash fields"};
            }
            if (fieldValue(fields, 3U, Field::ArgonVersion) != "19"_el) {
                throw err::ParseError{"Unsupported Argon2 version"};
            }
            const auto memory = parseCanonicalInteger<uint32_t>(fieldValue(fields, 4U, Field::Memory));
            const auto passes = parseCanonicalInteger<uint32_t>(fieldValue(fields, 5U, Field::Passes));
            const auto lanes = parseCanonicalInteger<uint32_t>(fieldValue(fields, 6U, Field::Parallelization));
            policy = PasswordHashPolicy{unsafe::UnsafeCustomPasswordHashParameters::argon2id(memory, passes, lanes)};
            modeIndex = 7U;
        } else if (algorithmText == "scrypt"_el) {
            if (fields.count().toSizeT() != 9U && fields.count().toSizeT() != 10U) {
                throw err::ParseError{"Unexpected scrypt password hash fields"};
            }
            const auto cost = parseCanonicalInteger<uint64_t>(fieldValue(fields, 3U, Field::Cost));
            const auto blockSize = parseCanonicalInteger<uint32_t>(fieldValue(fields, 4U, Field::BlockSize));
            const auto parallelization =
                parseCanonicalInteger<uint32_t>(fieldValue(fields, 5U, Field::Parallelization));
            policy = PasswordHashPolicy{
                unsafe::UnsafeCustomPasswordHashParameters::scrypt(cost, blockSize, parallelization)};
            modeIndex = 6U;
        } else {
            throw err::ParseError{"Unsupported password hashing algorithm"};
        }

        const auto mode = fieldValue(fields, modeIndex, Field::Mode);
        const auto keyed = mode == "hmac-sha256"_el;
        if (!keyed && mode != "none"_el) {
            throw err::ParseError{"Unsupported password hash protection mode"};
        }
        auto index = modeIndex + 1U;
        auto keyIdentifier = std::optional<text::String>{};
        const auto &rawFields = fields.toRawValue();
        const auto hasKeyIdentifier =
            index < rawFields.size() && rawFields[index].keyIndex() == static_cast<int>(Field::KeyIdentifier);
        if (keyed && hasKeyIdentifier) {
            const auto identifier = fieldValue(fields, index++, Field::KeyIdentifier);
            if (!validKeyIdentifier(identifier)) {
                throw err::ParseError{"Invalid password hash key identifier"};
            }
            keyIdentifier = identifier;
        }
        if (!keyed && hasKeyIdentifier) {
            throw err::ParseError{"Unkeyed password hashes cannot contain a key identifier"};
        }
        const auto saltText = fieldValue(fields, index++, Field::Salt);
        const auto verifierText = fieldValue(fields, index++, Field::Data);
        if (index != fields.count().toSizeT()) {
            throw err::ParseError{"Password hash contains unknown fields"};
        }
        const auto salt = decodePasswordHashBytes(saltText, unit::ByteLength{16U});
        const auto verifier = decodePasswordHashBytes(verifierText, unit::ByteLength{32U});
        const auto header = buildPasswordHashHeader(policy, keyed, keyIdentifier, salt.span());
        const auto canonical = buildPasswordHashRecord(header, verifier.span());
        if (canonical != text) {
            throw err::ParseError{"Password hash record is not canonical"};
        }

        auto data = std::make_shared<PasswordHashData>();
        data->policy = policy;
        data->keyed = keyed;
        data->keyIdentifier = std::move(keyIdentifier);
        data->salt = salt;
        data->verifier = verifier;
        data->headerThroughSalt = header;
        data->canonical = canonical;
        return data;
    } catch (const err::ParseError &) {
        throw;
    } catch (...) {
        throw err::ParseError{"Malformed or unsafe password hash record"};
    }
}

}
