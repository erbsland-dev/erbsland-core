// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>

#include <array>
#include <span>
#include <string_view>

using namespace el::cryptology;

TESTED_TARGETS(Sha1 Sha2 Md5 Hasher)
class HashValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    void verifyHash(
        const HashAlgorithm algorithm, const el::mem::ByteBlock &message, const el::mem::ByteBlock &expectedDigest) {
        auto hasher = Hasher{algorithm};
        hasher.update(message);
        REQUIRE_EQUAL(hasher.finalize(), expectedDigest);
    }

public:
    void testRfc1321Md5() {
        static constexpr auto vectors = std::array<std::pair<std::string_view, std::string_view>, 7>{
            std::pair{"", "d41d8cd98f00b204e9800998ecf8427e"},
            std::pair{"a", "0cc175b9c0f1b6a831c399e269772661"},
            std::pair{"abc", "900150983cd24fb0d6963f7d28e17f72"},
            std::pair{"message digest", "f96b697d7cb7938d525a2f31aaf161d0"},
            std::pair{"abcdefghijklmnopqrstuvwxyz", "c3fcd3d76192e4007dfb496cca67e13b"},
            std::pair{
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", "d174ab98d277d9f5a5611c2c9f419d9f"},
            std::pair{
                "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
                "57edf4a22be3c955ac49da2e2107b67a"}};
        for (const auto &[message, digest] : vectors) {
            const auto bytes = el::mem::ByteBlock::fromSpan(std::span<const char>{message});
            WITH_CONTEXT(verifyHash(HashAlgorithm::Md5, bytes, bytesFromHex(digest)));
        }
    }

    void testPaddingBoundariesAndLifecycle() {
        for (
            const auto algorithm :
            {HashAlgorithm::Sha1,
                HashAlgorithm::Sha2_256,
                HashAlgorithm::Sha2_384,
                HashAlgorithm::Sha2_512,
                HashAlgorithm::Md5}) {
            for (const auto length : {55U, 56U, 63U, 64U, 111U, 112U, 127U, 128U}) {
                const auto message = el::mem::ByteBlock{el::unit::ByteLength{length}, el::mem::Byte{0xa5U}};
                auto whole = Hasher{algorithm};
                whole.update(message);
                const auto expected = whole.finalize();
                REQUIRE_EQUAL(whole.finalize(), expected);

                whole.reset();
                const auto bytes = message.span();
                for (const auto byte : bytes) {
                    whole.update(el::mem::ConstByteSpan{&byte, 1U});
                }
                REQUIRE_EQUAL(whole.finalize(), expected);
            }
        }
    }
};
