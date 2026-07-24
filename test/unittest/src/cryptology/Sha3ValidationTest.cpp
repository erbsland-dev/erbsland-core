// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>
#include <erbsland/cryptology/StdFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <format>
#include <fstream>
#include <utility>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(Hasher Sha3)
class Sha3ValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
public:
    struct TestFile {
        std::filesystem::path path;
        HashAlgorithm algorithm;
    };
    inline static auto nistFiles = std::array<TestFile, 6>{
        TestFile{"data/cryptology/sha3/SHA3_256LongMsg.rsp", HashAlgorithm::Sha3_256},
        TestFile{"data/cryptology/sha3/SHA3_256ShortMsg.rsp", HashAlgorithm::Sha3_256},
        TestFile{"data/cryptology/sha3/SHA3_384LongMsg.rsp", HashAlgorithm::Sha3_384},
        TestFile{"data/cryptology/sha3/SHA3_384ShortMsg.rsp", HashAlgorithm::Sha3_384},
        TestFile{"data/cryptology/sha3/SHA3_512LongMsg.rsp", HashAlgorithm::Sha3_512},
        TestFile{"data/cryptology/sha3/SHA3_512ShortMsg.rsp", HashAlgorithm::Sha3_512},
    };

    void verifyHash(
        HashAlgorithm algorithm, const el::mem::ByteBlock &message, const el::mem::ByteBlock &expectedDigest) {
        // Verify in one go.
        Hasher hash{algorithm};
        const auto messageSpan = message.span();
        hash.update(message);
        auto actualDigest = hash.finalize();
        REQUIRE_EQUAL(actualDigest, expectedDigest);
        // Verify in chunks.
        hash.reset();
        constexpr std::size_t chunkSize = 10;
        for (std::size_t i = 0; i < messageSpan.size(); i += chunkSize) {
            auto chunk = messageSpan.subspan(i, std::min(chunkSize, messageSpan.size() - i));
            hash.update(chunk);
        }
        actualDigest = hash.finalize();
        REQUIRE_EQUAL(actualDigest, expectedDigest);
    }

    void testZeroByte() {
        Hasher hash(HashAlgorithm::Sha3_256);
        hash.update(bytesFromHex("00"));
        auto digest = hash.finalize();
        auto expectedDigest = bytesFromHex("5d53469f20fef4f8eab52b88044ede69c77a6a68a60728609fc4a65ff531e7d0");
        REQUIRE_EQUAL(digest, expectedDigest);
    }

    void testFourByteMessage() {
        Hasher hash(HashAlgorithm::Sha3_256);
        hash.update(bytesFromHex("74657374"));
        auto digest = hash.finalize();
        auto expectedDigest = bytesFromHex("36f028580bb02cc8272a9a020f4200e346e276ae664e45ee80745574e2f5ab80");
        REQUIRE_EQUAL(digest, expectedDigest);
    }

    void verifyFile(const TestFile &file) {
        auto testFilePath = unitTestExecutablePath().parent_path() / file.path;
        auto stream = std::ifstream(testFilePath);
        REQUIRE(stream.is_open());
        std::string line;
        el::mem::ByteBlock message;
        std::size_t expectedMessageSize = 0;
        int testCount = 0;
        while (std::getline(stream, line)) {
            if (line.starts_with("Len = ")) {
                auto messageSizeInBits = std::stoi(line.substr(6));
                REQUIRE_EQUAL(messageSizeInBits % 8, 0);
                expectedMessageSize = messageSizeInBits / 8;
            } else if (line.starts_with("Msg = ")) {
                std::string_view msgView(line.begin() + 6, line.end());
                if (expectedMessageSize == 0) {
                    // Some test files encode zero length messages as "00".
                    message = {};
                } else {
                    message = bytesFromHex(msgView);
                    REQUIRE_EQUAL(expectedMessageSize, message.length().toSizeT());
                }
            } else if (line.starts_with("MD = ")) {
                auto digest = bytesFromHex(std::string_view(line.begin() + 5, line.end()));
                WITH_CONTEXT(verifyHash(file.algorithm, message, digest));
                testCount += 1;
            }
        }
        REQUIRE_GREATER(testCount, 50); // Sanity test. Fail if the test files aren't properly read.
    }

    void testSha3NIST() {
        for (const auto &nistFile : nistFiles) {
            runWithContext(
                SOURCE_LOCATION(),
                [&, this]() { verifyFile(nistFile); },
                [&, this]() -> std::string {
                    return std::format(
                        "Failed to verify file {} with algorithm {}", nistFile.path.string(), nistFile.algorithm);
                });
        }
    }
};
