// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

using namespace el::cryptology;

TESTED_TARGETS(Sha1 Sha2 Md5)
class HashValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    struct TestFile {
        std::filesystem::path path;
        HashAlgorithm algorithm;
    };

    inline static const auto messageFiles = std::array<TestFile, 8>{
        TestFile{"data/cryptology/sha/SHA1ShortMsg.rsp", HashAlgorithm::Sha1},
        TestFile{"data/cryptology/sha/SHA1LongMsg.rsp", HashAlgorithm::Sha1},
        TestFile{"data/cryptology/sha/SHA256ShortMsg.rsp", HashAlgorithm::Sha2_256},
        TestFile{"data/cryptology/sha/SHA256LongMsg.rsp", HashAlgorithm::Sha2_256},
        TestFile{"data/cryptology/sha/SHA384ShortMsg.rsp", HashAlgorithm::Sha2_384},
        TestFile{"data/cryptology/sha/SHA384LongMsg.rsp", HashAlgorithm::Sha2_384},
        TestFile{"data/cryptology/sha/SHA512ShortMsg.rsp", HashAlgorithm::Sha2_512},
        TestFile{"data/cryptology/sha/SHA512LongMsg.rsp", HashAlgorithm::Sha2_512},
    };

    inline static const auto monteCarloFiles = std::array<TestFile, 7>{
        TestFile{"data/cryptology/sha/SHA1Monte.rsp", HashAlgorithm::Sha1},
        TestFile{"data/cryptology/sha/SHA256Monte.rsp", HashAlgorithm::Sha2_256},
        TestFile{"data/cryptology/sha/SHA384Monte.rsp", HashAlgorithm::Sha2_384},
        TestFile{"data/cryptology/sha/SHA512Monte.rsp", HashAlgorithm::Sha2_512},
        TestFile{"data/cryptology/sha3/SHA3_256Monte.rsp", HashAlgorithm::Sha3_256},
        TestFile{"data/cryptology/sha3/SHA3_384Monte.rsp", HashAlgorithm::Sha3_384},
        TestFile{"data/cryptology/sha3/SHA3_512Monte.rsp", HashAlgorithm::Sha3_512},
    };

public:
    [[nodiscard]] static auto valueAfter(const std::string &line, const std::size_t offset) -> std::string_view {
        auto value = std::string_view{line}.substr(offset);
        if (!value.empty() && value.back() == '\r') {
            value.remove_suffix(1);
        }
        return value;
    }

    [[nodiscard]] auto pathFor(const TestFile &file) -> std::filesystem::path {
        return unitTestExecutablePath().parent_path() / file.path;
    }

    void verifyHash(
        const HashAlgorithm algorithm, const el::mem::ByteBlock &message, const el::mem::ByteBlock &expectedDigest) {
        auto hasher = Hasher{algorithm};
        hasher.update(message);
        REQUIRE_EQUAL(hasher.finalize(), expectedDigest);

        hasher.reset();
        const auto bytes = message.span();
        for (std::size_t offset = 0; offset < bytes.size();) {
            const auto chunkLength = std::min(((offset * 13U) % 97U) + 1U, bytes.size() - offset);
            hasher.update(bytes.subspan(offset, chunkLength));
            offset += chunkLength;
        }
        REQUIRE_EQUAL(hasher.finalize(), expectedDigest);
    }

    void verifyMessageFile(const TestFile &file) {
        auto stream = std::ifstream{pathFor(file)};
        REQUIRE(stream.is_open());
        auto line = std::string{};
        auto message = el::mem::ByteBlock{};
        auto expectedLength = std::size_t{};
        auto testCount = std::size_t{};
        while (std::getline(stream, line)) {
            if (line.starts_with("Len = ")) {
                const auto bitLength = static_cast<std::size_t>(std::stoull(line.substr(6)));
                REQUIRE_EQUAL(bitLength % 8U, 0U);
                expectedLength = bitLength / 8U;
            } else if (line.starts_with("Msg = ")) {
                message = expectedLength == 0U ? el::mem::ByteBlock{} : bytesFromHex(valueAfter(line, 6));
                REQUIRE_EQUAL(message.length().toSizeT(), expectedLength);
            } else if (line.starts_with("MD = ")) {
                WITH_CONTEXT(verifyHash(file.algorithm, message, bytesFromHex(valueAfter(line, 5))));
                ++testCount;
            }
        }
        REQUIRE_GREATER(testCount, 50U);
    }

    [[nodiscard]] auto monteCarloRound(const HashAlgorithm algorithm, const el::mem::ByteBlock &seed)
        -> el::mem::ByteBlock {
        if (algorithm == HashAlgorithm::Sha3_256 || algorithm == HashAlgorithm::Sha3_384 ||
            algorithm == HashAlgorithm::Sha3_512) {
            auto digest = seed;
            for (std::size_t iteration = 0; iteration < 1000U; ++iteration) {
                auto hasher = Hasher{algorithm};
                hasher.update(digest);
                digest = hasher.finalize();
            }
            return digest;
        }
        auto first = seed;
        auto second = seed;
        auto third = seed;
        for (std::size_t iteration = 0; iteration < 1000U; ++iteration) {
            auto hasher = Hasher{algorithm};
            hasher.update(first);
            hasher.update(second);
            hasher.update(third);
            auto next = hasher.finalize();
            first = std::move(second);
            second = std::move(third);
            third = std::move(next);
        }
        return third;
    }

    void verifyMonteCarloFile(const TestFile &file) {
        auto stream = std::ifstream{pathFor(file)};
        REQUIRE(stream.is_open());
        auto line = std::string{};
        auto seed = el::mem::ByteBlock{};
        auto checkpointCount = std::size_t{};
        while (std::getline(stream, line)) {
            if (line.starts_with("Seed = ")) {
                seed = bytesFromHex(valueAfter(line, 7));
            } else if (line.starts_with("MD = ")) {
                const auto expected = bytesFromHex(valueAfter(line, 5));
                seed = monteCarloRound(file.algorithm, seed);
                REQUIRE_EQUAL(seed, expected);
                ++checkpointCount;
            }
        }
        REQUIRE_EQUAL(checkpointCount, 100U);
    }

    void testNistShortAndLongMessages() {
        for (const auto &file : messageFiles) {
            WITH_CONTEXT(verifyMessageFile(file));
        }
    }

    void testNistMonteCarlo() {
        for (const auto &file : monteCarloFiles) {
            WITH_CONTEXT(verifyMonteCarloFile(file));
        }
    }

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
