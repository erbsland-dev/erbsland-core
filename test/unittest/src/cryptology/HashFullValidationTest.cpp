// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "CryptologyResponseReader.hpp"
#include "CryptologyTestHelper.hpp"

#include <erbsland/cryptology/Hasher.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>

using namespace el::cryptology;
using namespace el::text::literals;

TESTED_TARGETS(Sha1 Sha2 Sha3 Hasher)
class HashFullValidationTest final : public UNITTEST_SUBCLASS(CryptologyTestHelper) {
private:
    struct TestFile final {
        std::string_view path;
        HashAlgorithm algorithm;
        std::size_t records;
    };

    inline static constexpr auto messageFiles = std::array{
        TestFile{"data/cryptology/sha/SHA1ShortMsg.rsp", HashAlgorithm::Sha1, 65U},
        TestFile{"data/cryptology/sha/SHA1LongMsg.rsp", HashAlgorithm::Sha1, 64U},
        TestFile{"data/cryptology/sha/SHA256ShortMsg.rsp", HashAlgorithm::Sha2_256, 65U},
        TestFile{"data/cryptology/sha/SHA256LongMsg.rsp", HashAlgorithm::Sha2_256, 64U},
        TestFile{"data/cryptology/sha/SHA384ShortMsg.rsp", HashAlgorithm::Sha2_384, 129U},
        TestFile{"data/cryptology/sha/SHA384LongMsg.rsp", HashAlgorithm::Sha2_384, 128U},
        TestFile{"data/cryptology/sha/SHA512ShortMsg.rsp", HashAlgorithm::Sha2_512, 129U},
        TestFile{"data/cryptology/sha/SHA512LongMsg.rsp", HashAlgorithm::Sha2_512, 128U},
        TestFile{"data/cryptology/sha3/SHA3_256ShortMsg.rsp", HashAlgorithm::Sha3_256, 137U},
        TestFile{"data/cryptology/sha3/SHA3_256LongMsg.rsp", HashAlgorithm::Sha3_256, 100U},
        TestFile{"data/cryptology/sha3/SHA3_384ShortMsg.rsp", HashAlgorithm::Sha3_384, 105U},
        TestFile{"data/cryptology/sha3/SHA3_384LongMsg.rsp", HashAlgorithm::Sha3_384, 100U},
        TestFile{"data/cryptology/sha3/SHA3_512ShortMsg.rsp", HashAlgorithm::Sha3_512, 73U},
        TestFile{"data/cryptology/sha3/SHA3_512LongMsg.rsp", HashAlgorithm::Sha3_512, 100U},
    };

    inline static constexpr auto monteCarloFiles = std::array{
        TestFile{"data/cryptology/sha/SHA1Monte.rsp", HashAlgorithm::Sha1, 100U},
        TestFile{"data/cryptology/sha/SHA256Monte.rsp", HashAlgorithm::Sha2_256, 100U},
        TestFile{"data/cryptology/sha/SHA384Monte.rsp", HashAlgorithm::Sha2_384, 100U},
        TestFile{"data/cryptology/sha/SHA512Monte.rsp", HashAlgorithm::Sha2_512, 100U},
        TestFile{"data/cryptology/sha3/SHA3_256Monte.rsp", HashAlgorithm::Sha3_256, 100U},
        TestFile{"data/cryptology/sha3/SHA3_384Monte.rsp", HashAlgorithm::Sha3_384, 100U},
        TestFile{"data/cryptology/sha3/SHA3_512Monte.rsp", HashAlgorithm::Sha3_512, 100U},
    };

    void verifyHash(
        const HashAlgorithm algorithm, const el::mem::ByteBlock &message, const el::mem::ByteBlock &expectedDigest) {
        auto hasher = Hasher{algorithm};
        hasher.update(message);
        REQUIRE_EQUAL(hasher.finalize(), expectedDigest);

        hasher.reset();
        for (auto offset = std::size_t{}; offset < message.length().toSizeT();) {
            const auto chunkLength = std::min(((offset * 13U) % 97U) + 1U, message.length().toSizeT() - offset);
            hasher.update(message.span().subspan(offset, chunkLength));
            offset += chunkLength;
        }
        REQUIRE_EQUAL(hasher.finalize(), expectedDigest);
    }

    [[nodiscard]] static auto monteCarloRound(const HashAlgorithm algorithm, const el::mem::ByteBlock &seed)
        -> el::mem::ByteBlock {
        if (algorithm == HashAlgorithm::Sha3_256 || algorithm == HashAlgorithm::Sha3_384 ||
            algorithm == HashAlgorithm::Sha3_512) {
            auto digest = seed;
            for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
                auto hasher = Hasher{algorithm};
                hasher.update(digest);
                digest = hasher.finalize();
            }
            return digest;
        }
        auto first = seed;
        auto second = seed;
        auto third = seed;
        for (auto iteration = std::size_t{}; iteration < 1000U; ++iteration) {
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

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNistShortAndLongMessages() {
        for (const auto &file : messageFiles) {
            const auto records = CryptologyResponseReader{el::Path{std::filesystem::path{file.path}}}.read();
            REQUIRE_EQUAL(records.size(), file.records);
            for (const auto &record : records) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void {
                        record.requireAllowedSettings({"L"_el});
                        record.requireAllowedValues({"Len"_el, "Msg"_el, "MD"_el});
                        record.requireValues({"Len"_el, "Msg"_el, "MD"_el});
                        const auto bitLength = record.unsignedValue("Len"_el);
                        REQUIRE_EQUAL(bitLength % 8U, 0U);
                        const auto message =
                            bitLength == 0U ? el::mem::ByteBlock{} : bytesFromHex(record.value("Msg"_el));
                        REQUIRE_EQUAL(message.length().toSizeT(), bitLength / 8U);
                        verifyHash(file.algorithm, message, bytesFromHex(record.value("MD"_el)));
                    },
                    [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
            }
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testNistMonteCarlo() {
        for (const auto &file : monteCarloFiles) {
            const auto records = CryptologyResponseReader{el::Path{std::filesystem::path{file.path}}}.read();
            REQUIRE_EQUAL(records.size(), file.records + 1U);
            records.front().requireAllowedSettings({"L"_el});
            records.front().requireAllowedValues({"Seed"_el});
            records.front().requireValues({"Seed"_el});
            auto seed = bytesFromHex(records.front().value("Seed"_el));
            for (const auto &record : std::span{records}.subspan(1U)) {
                runWithContext(
                    SOURCE_LOCATION(),
                    [&]() -> void {
                        record.requireAllowedSettings({"L"_el});
                        record.requireAllowedValues({"COUNT"_el, "MD"_el});
                        record.requireValues({"COUNT"_el, "MD"_el});
                        REQUIRE_EQUAL(record.unsignedValue("COUNT"_el), record.index - 1U);
                        seed = monteCarloRound(file.algorithm, seed);
                        REQUIRE_EQUAL(seed, bytesFromHex(record.value("MD"_el)));
                    },
                    [&]() -> std::string { return el::StringConverter{record.diagnostic()}.toStdString(); });
            }
        }
    }
};
