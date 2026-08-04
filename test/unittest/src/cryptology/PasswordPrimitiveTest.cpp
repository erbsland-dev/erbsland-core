// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/impl/algorithm/Argon2id.hpp>
#include <erbsland/cryptology/impl/algorithm/Blake2b.hpp>
#include <erbsland/cryptology/impl/algorithm/HmacSha256.hpp>
#include <erbsland/cryptology/impl/algorithm/Scrypt.hpp>
#include <erbsland/mem/ByteArray.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/impl/UnsafeByteArrayAccess.hpp>

#include <span>
#include <string_view>

using namespace erbsland::cryptology;
using namespace erbsland::unit;
using namespace erbsland::mem;
using erbsland::cryptology::impl::Argon2id;
using erbsland::cryptology::impl::Blake2b;
using erbsland::cryptology::impl::hmacSha256;
using erbsland::cryptology::impl::pbkdf2HmacSha256;
using erbsland::cryptology::impl::salsa20_8;
using erbsland::cryptology::impl::scrypt;

TESTED_TARGETS(Blake2b)
class PasswordPrimitiveTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    void testBlake2bRfcVectors() {
        requireBlake2b(
            "",
            "786a02f742015903c6c6fd852552d272"
            "912f4740e15847618a86e217f71f5419"
            "d25e1031afee585313896444934eb04b"
            "903a685b1448b755d56f701afe9be2ce");
        requireBlake2b(
            "abc",
            "ba80a53f981c4d0d6a2797b69f12f6e"
            "94c212f14685ac4b74b12bb6fdbffa2"
            "d17d87c5392aab792dc252d5de4533cc"
            "9518d38aa8dbf1925ab92386edd4009923");
    }

    void testBlake2bSecureErasePreservesDigestLength() {
        const auto secret = sensitiveBytes("secret-prefix");
        const auto message = sensitiveBytes("known-message");

        auto erased = Blake2b{ByteLength{32U}};
        erased.update(raw(secret));
        erased.secureErase();
        erased.update(raw(message));
        const auto erasedDigest = ByteBlock{erased.digest()};
        REQUIRE(erasedDigest.isSensitive());

        auto fresh = Blake2b{ByteLength{32U}};
        fresh.update(raw(message));
        const auto freshDigest = ByteBlock{fresh.digest()};
        REQUIRE_EQUAL(erasedDigest.length(), ByteLength{32U});
        REQUIRE_EQUAL(erasedDigest, freshDigest);

        erased.secureErase();
        erased.update(raw(message));
        const auto erasedDigestAfterReset = ByteBlock{erased.digest()};
        REQUIRE_EQUAL(erasedDigestAfterReset, freshDigest);
    }

    void testHmacSha256RfcVector() {
        auto key = ByteBlockEditor{ByteLength{20U}};
        for (auto index = ByteIndex::zero(); index < ByteIndex{20U}; ++index) {
            key.set(index, Byte{0x0bU});
        }
        const auto message = sensitiveBytes("Hi There");
        const auto actual = ByteBlock{hmacSha256(raw(key), {raw(message)})};
        REQUIRE(actual.isSensitive());
        const auto expected =
            ByteBlock{bytesFromHex("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7")};
        REQUIRE_EQUAL(actual, expected);
    }

    void testPbkdf2HmacSha256Vectors() {
        const auto password = sensitiveBytes("password");
        const auto salt = sensitiveBytes("salt");
        requirePbkdf2(password, salt, 1U, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
        requirePbkdf2(password, salt, 2U, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
    }

    void testSalsa20_8RfcVector() {
        const auto input = bytesFromHex(
            "7e879a214f3ec9867ca940e641718f26"
            "baee555b8c61c1b50df846116dcd3b1d"
            "ee24f319df9b3d8514121e4b5ac5aa32"
            "76021d2909c74829edebc68db8b8c25e");
        auto block = el::mem::ByteArray<64>{};
        block.overwrite(input.span());
        salsa20_8(el::mem::impl::UnsafeByteArrayAccess{block}.writableData());
        const auto expected = bytesFromHex(
            "a41f859c6608cc993b81cacb020cef05"
            "044b2181a2fd337dfd7b1c6396682f29"
            "b4393168e3c9e6bcfe6bc5b7a06d96ba"
            "e424cc102c91745c24ad673dc7618f81");
        const auto actual = el::mem::ByteBlock{block};
        REQUIRE_EQUAL(actual, expected);
    }

    void testScryptRfcVector() {
        const auto empty = ByteBlock{};
        const auto actual = ByteBlock{scrypt(raw(empty), raw(empty), 16U, 1U, 1U, 64U)};
        REQUIRE(actual.isSensitive());
        const auto expected = ByteBlock{bytesFromHex(
            "77d6576238657b203b19ca42c18a0497"
            "f16b4844e3074ae8dfdffa3fede21442"
            "fcd0069ded0948f8326a753a0fc81f17"
            "e8d3e0fb2e0d3628cf35e20c38d18906")};
        REQUIRE_EQUAL(actual, expected);
    }

    void testArgon2idRfcVector() {
        const auto password = repeatedBytes(32U, 0x01U);
        const auto salt = repeatedBytes(16U, 0x02U);
        const auto secret = repeatedBytes(8U, 0x03U);
        const auto associated = repeatedBytes(12U, 0x04U);
        const auto argon2id = Argon2id{{.memoryKiB = 32U, .passes = 3U, .lanes = 4U, .outputLength = 32U}};
        const auto actual = ByteBlock{argon2id.derive(raw(password), raw(salt), raw(secret), raw(associated))};
        REQUIRE(actual.isSensitive());
        const auto expected =
            ByteBlock{bytesFromHex("0d640df58d78766c08c037a34a8b53c9d01ef0452d75b65eb52520e96b01e659")};
        REQUIRE_EQUAL(actual, expected);
    }

private:
    void requireBlake2b(const std::string_view message, const std::string_view expectedHex) {
        const auto input = sensitiveBytes(message);
        const auto actual = ByteBlock{Blake2b{ByteLength{64U}}.digest(raw(input))};
        REQUIRE(actual.isSensitive());
        const auto expected = ByteBlock{bytesFromHex(expectedHex)};
        REQUIRE_EQUAL(actual, expected);
    }

    void requirePbkdf2(
        const ByteBlock &password,
        const ByteBlock &salt,
        const uint32_t iterations,
        const std::string_view expectedHex) {
        const auto actual = ByteBlock{pbkdf2HmacSha256(raw(password), raw(salt), iterations, 32U)};
        REQUIRE(actual.isSensitive());
        const auto expected = ByteBlock{bytesFromHex(expectedHex)};
        REQUIRE_EQUAL(actual, expected);
    }

    [[nodiscard]] static auto sensitiveBytes(const std::string_view text) -> ByteBlock {
        auto result =
            ByteBlock::fromSpan(std::span<const uint8_t>{reinterpret_cast<const uint8_t *>(text.data()), text.size()});
        result.markAsSensitive();
        return result;
    }

    [[nodiscard]] static auto repeatedBytes(const std::size_t size, const uint8_t value) -> ByteBlock {
        auto result = ByteBlockEditor{ByteLength::fromSizeT(size)};
        result.markAsSensitive();
        result.fill(Byte{value});
        return result;
    }

    template <typename T>
    [[nodiscard]] static auto raw(const T &block) noexcept -> std::span<const Byte> {
        return block.span();
    }
};
