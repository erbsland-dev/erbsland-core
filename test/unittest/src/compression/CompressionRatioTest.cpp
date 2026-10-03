// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../ParallelTestWork.hpp"

#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteCompressor.hpp>
#include <erbsland/ByteDecompressor.hpp>
#include <erbsland/CompressionAlgorithm.hpp>
#include <erbsland/CompressionFormat.hpp>
#include <erbsland/CompressionLevel.hpp>
#include <erbsland/DecompressionOptions.hpp>
#include <erbsland/StringConverter.hpp>
#include <erbsland/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ByteCompressor ByteDecompressor)
class CompressionRatioTest final : public el::UnitTest {
private:
    struct TestCase final {
        el::compression::CompressionAlgorithm algorithm;
        el::compression::CompressionLevel level;
        std::size_t size;
    };
    struct Result final {
        bool roundTrip;
        uint64_t compressedLength;
        uint64_t sourceLength;
    };

public:
    void testCompressionRatioLight() {
        using namespace el::compression;
        for (const auto algorithm : CompressionAlgorithm::all()) {
            WITH_CONTEXT(requireCompressionRatio(algorithm, CompressionLevel::Default, 8192U));
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCompressionRatioAllLevels() {
        auto cases = std::vector<TestCase>{};
        for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
            for (
                const auto level :
                {el::compression::CompressionLevel::Fastest,
                    el::compression::CompressionLevel::Fast,
                    el::compression::CompressionLevel::Default,
                    el::compression::CompressionLevel::High,
                    el::compression::CompressionLevel::Highest}) {
                cases.push_back({algorithm, level, 8192U});
            }
        }
        WITH_CONTEXT(requireCases(cases));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCompressionRatioAcrossWindows() {
        auto cases = std::vector<TestCase>{};
        using namespace el::compression;
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto level : {CompressionLevel::Fastest, CompressionLevel::Default}) {
                auto size = 3U * 1024U * 1024U;
                if (algorithm == CompressionAlgorithm::Lzma && level == CompressionLevel::Default) {
                    size = 17U * 1024U * 1024U;
                } else if (algorithm == CompressionAlgorithm::Zstandard && level == CompressionLevel::Default) {
                    size = 9U * 1024U * 1024U;
                }
                cases.push_back({algorithm, level, size});
            }
        }
        WITH_CONTEXT(requireCases(cases));
    }

private:
    [[nodiscard]] static auto computeRatio(const TestCase &testCase) -> Result {
        const auto &[algorithm, level, size] = testCase;
        using namespace el::compression;
        auto bytes = std::vector<el::mem::Byte>(size);
        auto state = uint32_t{1234567U};
        for (auto index = std::size_t{}; index < size; ++index) {
            state ^= state << 13U;
            state ^= state >> 17U;
            state ^= state << 5U;
            // Alternate fresh entropy, long-distance copies, and short repeating patterns.
            // This also exercises scratch reuse after an unprofitable block candidate.
            const auto region = index / 131072U % 4U;
            if (region == 0U && size > 8192U) {
                bytes[index] = el::mem::Byte::fromCroppedUInt32(state);
            } else if (region == 1U) {
                bytes[index] = bytes[index - 131072U];
            } else {
                bytes[index] = el::mem::Byte::fromCroppedUInt64(index % 251U);
            }
        }
        const auto source = el::mem::ByteBlock::fromSpan(el::mem::ConstByteSpan{bytes});
        const auto compressed = ByteCompressor{algorithm, CompressionFormat::Raw, level}.compress(source);
        const auto decoded =
            ByteDecompressor{
                algorithm, CompressionFormat::Raw, DecompressionOptions{}.setExpectedOutputLength(source.length())}
                .decompress(compressed);
        return {decoded == source, compressed.length().toRawValue(), source.length().toRawValue()};
    }

    void requireResult(const TestCase &testCase, const parallel_test_work::Result<Result> &result) {
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
                REQUIRE_NOTHROW(result.value());
                const auto &value = result.value();
                REQUIRE(value.roundTrip);
                if (testCase.algorithm != el::compression::CompressionAlgorithm::Deflate ||
                    testCase.level != el::compression::CompressionLevel::Fastest) {
                    REQUIRE_LESS(value.compressedLength, value.sourceLength * 3U / 4U);
                }
            },
            [&]() -> std::string {
                return el::text::StringConverter{
                    el::text::StringFormat{"algorithm={} level={} input-bytes={}"_el}.build(
                        testCase.algorithm.toString(), static_cast<unsigned>(testCase.level), testCase.size)}
                    .toStdString();
            });
    }

    void requireCases(const std::vector<TestCase> &cases) {
        // Two workers bound the simultaneous large codec workspaces and input buffers.
        const auto results = parallel_test_work::run(
            cases.size(), [&](const std::size_t index) -> Result { return computeRatio(cases[index]); }, 2U);
        for (auto index = std::size_t{}; index < cases.size(); ++index) {
            WITH_CONTEXT(requireResult(cases[index], results[index]));
        }
    }

    void requireCompressionRatio(
        el::compression::CompressionAlgorithm algorithm, el::compression::CompressionLevel level, std::size_t size) {
        const auto testCase = TestCase{algorithm, level, size};
        const auto results = parallel_test_work::run(1U, [&](std::size_t) -> Result { return computeRatio(testCase); });
        WITH_CONTEXT(requireResult(testCase, results.front()));
    }
};
