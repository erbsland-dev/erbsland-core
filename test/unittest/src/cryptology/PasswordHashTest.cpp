// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/PasswordHash.hpp>
#include <erbsland/cryptology/PasswordHasher.hpp>
#include <erbsland/cryptology/PasswordHashKey.hpp>
#include <erbsland/cryptology/PasswordHashPolicy.hpp>
#include <erbsland/cryptology/unsafe/UnsafeCustomPasswordHashParameters.hpp>
#include <erbsland/cryptology/unsafe/UnsafeNoPasswordHashKey.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/util/List.hpp>

#include <array>
#include <string>
#include <string_view>

using namespace el::cryptology;
using el::mem::ByteBlock;
using el::mem::ByteBlockEditor;
using el::text::String;

namespace {

auto testKey(const uint8_t value, const std::string_view identifier = {}) -> PasswordHashKey {
    auto bytes = ByteBlockEditor{el::unit::ByteLength{32U}};
    for (auto index = el::unit::ByteIndex{0U}; index < el::unit::ByteIndex{32U}; ++index) {
        bytes.set(index, el::mem::Byte{value});
    }
    if (identifier.empty()) {
        return PasswordHashKey{ByteBlock{bytes}};
    }
    return PasswordHashKey::identified(el::text::String{identifier}, ByteBlock{bytes});
}

auto testPassword(const std::string_view password = "correct horse battery staple") -> String {
    auto result = String{password};
    result.markAsSensitive();
    return result;
}

auto fastArgon(const uint32_t memory = 32U, const uint32_t passes = 1U) -> PasswordHashPolicy {
    return PasswordHashPolicy{unsafe::UnsafeCustomPasswordHashParameters::argon2id(memory, passes, 1U)};
}

auto fastScrypt() -> PasswordHashPolicy {
    return PasswordHashPolicy{unsafe::UnsafeCustomPasswordHashParameters::scrypt(16U, 1U, 1U)};
}

static_assert(!std::is_convertible_v<PasswordVerification, bool>);

}

TESTED_TARGETS(
    PasswordHashPolicy PasswordHashKey PasswordHash PasswordHasher PasswordVerification UnsafeNoPasswordHashKey)
class PasswordHasherTest final : public el::UnitTest {
public:
    void testSafePresets() {
        const auto recommended = PasswordHashPolicy::recommended();
        REQUIRE_EQUAL(recommended.algorithm(), PasswordHashAlgorithm::Argon2id);
        REQUIRE_EQUAL(recommended.memoryKiB(), 64U * 1024U);
        REQUIRE_EQUAL(recommended.passes(), 3U);
        REQUIRE_EQUAL(recommended.lanes(), 4U);
        REQUIRE_EQUAL(recommended.saltLength(), el::unit::ByteLength{16U});
        REQUIRE_EQUAL(recommended.outputLength(), el::unit::ByteLength{32U});

        const auto lowMemory = PasswordHashPolicy::lowMemory();
        REQUIRE_EQUAL(lowMemory.memoryKiB(), 19U * 1024U);
        REQUIRE_EQUAL(lowMemory.passes(), 2U);
        REQUIRE_EQUAL(lowMemory.lanes(), 1U);

        const auto scryptPolicy = PasswordHashPolicy::scrypt();
        REQUIRE_EQUAL(scryptPolicy.algorithm(), PasswordHashAlgorithm::Scrypt);
        REQUIRE_EQUAL(scryptPolicy.scryptCost(), uint64_t{1U} << 17U);
        REQUIRE_EQUAL(scryptPolicy.scryptBlockSize(), 8U);
        REQUIRE_EQUAL(scryptPolicy.scryptParallelization(), 1U);
    }

    void testUnsafeParametersAreStillBounded() {
        REQUIRE_NOTHROW(fastArgon());
        REQUIRE_NOTHROW(fastScrypt());
        REQUIRE_THROWS_AS(el::err::ParameterError, unsafe::UnsafeCustomPasswordHashParameters::argon2id(7U, 1U, 1U));
        REQUIRE_THROWS_AS(el::err::ParameterError, unsafe::UnsafeCustomPasswordHashParameters::argon2id(32U, 11U, 1U));
        REQUIRE_THROWS_AS(el::err::ParameterError, unsafe::UnsafeCustomPasswordHashParameters::scrypt(15U, 1U, 1U));
    }
    void testKeyValidationAndIdentifiers() {
        const auto unnamed = testKey(1U);
        REQUIRE_FALSE(unnamed.isIdentified());

        const auto named = testKey(2U, "current-2026");
        REQUIRE(named.isIdentified());
        REQUIRE_EQUAL(*named.identifier(), el::text::String{"current-2026"});

        auto keyBytes = ByteBlock{ByteBlockEditor{el::unit::ByteLength{32U}}};
        const auto markedKey = PasswordHashKey{keyBytes};
        REQUIRE(keyBytes.isSensitive());
        static_cast<void>(markedKey);

        auto shortBytes = ByteBlockEditor{el::unit::ByteLength{31U}};
        REQUIRE_THROWS_AS(el::err::ParameterError, PasswordHashKey{ByteBlock{shortBytes}});
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            PasswordHashKey::identified(
                el::text::String{"contains,comma"}, ByteBlock{ByteBlockEditor{el::unit::ByteLength{32U}}}));
    }

    void testHashSerializeAndVerify() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto hasher = PasswordHasher{testKey(1U), fastArgon()};
        const auto password = testPassword();
        const auto first = hasher.hash(password);
        const auto second = hasher.hash(password);

        REQUIRE(first.isValid());
        REQUIRE(first.isKeyed());
        REQUIRE_EQUAL(first.algorithm(), PasswordHashAlgorithm::Argon2id);
        REQUIRE_FALSE(first.keyIdentifier().has_value());
        REQUIRE(first.toString() != second.toString());

        const auto parsed = PasswordHash::fromStringOrThrow(first.toString());
        REQUIRE_EQUAL(parsed.toString(), first.toString());
        const auto accepted = hasher.verify(password, parsed);
        REQUIRE(accepted.isAccepted());
        REQUIRE_FALSE(accepted.replacementHash().has_value());

        const auto rejected = hasher.verify(testPassword("wrong password"), parsed);
        REQUIRE(rejected.isRejected());
        REQUIRE_FALSE(rejected.replacementHash().has_value());
        REQUIRE(PasswordHasher{testKey(9U), fastArgon()}.verify(password, parsed).isRejected());

        auto tamperedRecord = el::text::StringConverter{first.toString()}.toStdString();
        const auto memoryField = tamperedRecord.find(",m:32,");
        REQUIRE(memoryField != std::string::npos);
        tamperedRecord.replace(memoryField, std::string_view{",m:32,"}.size(), ",m:40,");
        const auto tampered = PasswordHash::fromStringOrThrow(el::text::String{tamperedRecord});
        REQUIRE(hasher.verify(password, tampered).isRejected());
    }

    void testEmptyPasswordAndScrypt() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto hasher = PasswordHasher{testKey(1U), fastScrypt()};
        const auto empty = String{};
        const auto hash = hasher.hash(empty);
        REQUIRE_EQUAL(hash.algorithm(), PasswordHashAlgorithm::Scrypt);
        REQUIRE(hasher.verify(empty, hash).isAccepted());
        REQUIRE(hasher.verify(testPassword("not empty"), hash).isRejected());
    }

    void testUnkeyedMigration() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto password = testPassword();
        const auto unkeyed = PasswordHasher{unsafe::UnsafeNoPasswordHashKey::acknowledgeRisk(), fastArgon()};
        const auto oldHash = unkeyed.hash(password);
        REQUIRE_FALSE(oldHash.isKeyed());
        REQUIRE(unkeyed.verify(password, oldHash).isAccepted());

        const auto keyed = PasswordHasher{testKey(2U), fastArgon()};
        const auto migrated = keyed.verify(password, oldHash);
        REQUIRE(migrated.isAccepted());
        REQUIRE(migrated.replacementHash().has_value());
        REQUIRE(migrated.replacementHash()->isKeyed());
        REQUIRE(keyed.verify(password, *migrated.replacementHash()).isAccepted());
    }

    void testKeyRotationAndUnknownKey() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto password = testPassword();
        const auto oldHasher = PasswordHasher{testKey(1U, "old"), fastArgon()};
        const auto oldHash = oldHasher.hash(password);
        const auto rotating = PasswordHasher::withKeyRotation(
            testKey(2U, "new"), el::util::List<PasswordHashKey>{testKey(1U, "old")}, fastArgon());
        const auto result = rotating.verify(password, oldHash);
        REQUIRE(result.isAccepted());
        REQUIRE(result.replacementHash().has_value());
        REQUIRE_EQUAL(*result.replacementHash()->keyIdentifier(), el::text::String{"new"});

        const auto missing =
            PasswordHasher::withKeyRotation(testKey(3U, "other"), el::util::List<PasswordHashKey>{}, fastArgon());
        REQUIRE(missing.verify(password, oldHash).isRejected());

        const auto legacyHash = PasswordHasher{testKey(4U), fastArgon()}.hash(password);
        const auto legacyRotation = PasswordHasher::withKeyRotation(
            testKey(5U, "active"), el::util::List<PasswordHashKey>{testKey(4U)}, fastArgon());
        REQUIRE(legacyRotation.verify(password, legacyHash).replacementHash().has_value());
    }

    void testPolicyAndAlgorithmMigration() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto password = testPassword();
        const auto oldHash = PasswordHasher{testKey(1U), fastScrypt()}.hash(password);
        const auto current = PasswordHasher{testKey(1U), fastArgon(40U, 2U)};
        const auto result = current.verify(password, oldHash);
        REQUIRE(result.isAccepted());
        REQUIRE(result.replacementHash().has_value());
        REQUIRE_EQUAL(result.replacementHash()->algorithm(), PasswordHashAlgorithm::Argon2id);
    }

    void testMalformedSentinelAndLimits() {
        const auto hasher = PasswordHasher{testKey(1U), fastArgon()};
        const auto password = testPassword();
        const auto requireInvalid = [this](const std::string_view record) {
            const auto text = el::text::String{record};
            REQUIRE_FALSE(PasswordHash::fromString(text).isValid());
            REQUIRE_THROWS_AS(el::err::ParseError, PasswordHash::fromStringOrThrow(text));
        };
        const auto invalid = PasswordHash::fromString(el::text::String{"not-a-password-hash"});
        REQUIRE_FALSE(invalid.isValid());
        REQUIRE(invalid.toString().isEmpty());
        REQUIRE_THROWS_AS(el::err::LogicError, invalid.algorithm());
        REQUIRE_THROWS_AS(
            el::err::ParseError, PasswordHash::fromStringOrThrow(el::text::String{"not-a-password-hash"}));
        REQUIRE(hasher.verify(password, invalid).isRejected());

        const auto oversizedRecord = el::text::String{std::string(513U, 'x')};
        REQUIRE_FALSE(PasswordHash::fromString(oversizedRecord).isValid());

        const auto salt = std::string(22U, 'A');
        const auto digest = std::string(43U, 'A');
        const auto prefix = std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:8,t:1,p:1,x:none,s:"};
        const auto validRecord = prefix + salt + ",d:" + digest;
        REQUIRE(PasswordHash::fromString(el::text::String{validRecord}).isValid());
        const auto malformedRecords = std::array{
            std::string{"f:el-password-hash,v:2,a:argon2id,av:19,m:8,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:unknown,av:19,m:8,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:16,m:8,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:08,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:8,m:8,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:1048577,t:1,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:8,t:11,p:1,x:none,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:136,t:1,p:17,x:none,s:"} + salt + ",d:" + digest,
            prefix + salt + "==,d:" + digest,
            prefix + std::string(21U, 'A') + ",d:" + digest,
            prefix + salt + ",d:" + std::string(42U, 'A') + "*",
            prefix + salt + ",d:" + digest + ",z:unknown",
            std::string{"f:el-password-hash,v:1,a:argon2id,av:19,m:8,t:1,p:1,x:none,i:key,s:"} + salt + ",d:" + digest,
            std::string{"f:el-password-hash,v:1,a:scrypt,n:1048576,r:2,p:1,x:none,s:"} + salt + ",d:" + digest,
        };
        for (const auto &record : malformedRecords) {
            requireInvalid(record);
        }

        const auto oversizedPassword = String{std::string(1024U * 1024U + 1U, 'x')};
        REQUIRE_THROWS_AS(el::err::ParameterError, hasher.hash(oversizedPassword));
        REQUIRE(hasher.verify(oversizedPassword, invalid).isRejected());
    }

    void testProductionPresetSmoke() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto password = testPassword("production preset smoke");
        const auto argonHash = PasswordHasher{testKey(1U), PasswordHashPolicy::recommended()}.hash(password);
        REQUIRE(argonHash.isValid());
        const auto scryptHash = PasswordHasher{testKey(1U), PasswordHashPolicy::scrypt()}.hash(password);
        REQUIRE(scryptHash.isValid());
    }
};
