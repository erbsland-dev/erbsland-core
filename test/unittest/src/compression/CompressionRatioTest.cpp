// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/compression/CompressionFormat.hpp>
#include <erbsland/compression/CompressionLevel.hpp>
#include <erbsland/compression/DecompressionOptions.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ByteCompressor ByteDecompressor)
class CompressionRatioTest final : public el::UnitTest {
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
        for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
            for (
                const auto level :
                {el::compression::CompressionLevel::Fastest,
                    el::compression::CompressionLevel::Fast,
                    el::compression::CompressionLevel::Default,
                    el::compression::CompressionLevel::High,
                    el::compression::CompressionLevel::Highest}) {
                WITH_CONTEXT(requireCompressionRatio(algorithm, level, 8192U));
            }
        }
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testCompressionRatioAcrossWindows() {
        using namespace el::compression;
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto level : {CompressionLevel::Fastest, CompressionLevel::Default}) {
                auto size = 3U * 1024U * 1024U;
                if (algorithm == CompressionAlgorithm::Lzma && level == CompressionLevel::Default) {
                    size = 17U * 1024U * 1024U;
                } else if (algorithm == CompressionAlgorithm::Zstandard && level == CompressionLevel::Default) {
                    size = 9U * 1024U * 1024U;
                }
                WITH_CONTEXT(requireCompressionRatio(algorithm, level, size));
            }
        }
    }

private:
    void requireCompressionRatio(
        el::compression::CompressionAlgorithm algorithm, el::compression::CompressionLevel level, std::size_t size) {
        runWithContext(
            SOURCE_LOCATION(),
            [&]() -> void {
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
                        algorithm,
                        CompressionFormat::Raw,
                        DecompressionOptions{}.setExpectedOutputLength(source.length())}
                        .decompress(compressed);
                REQUIRE_EQUAL(decoded, source);
                if (algorithm != CompressionAlgorithm::Deflate || level != CompressionLevel::Fastest) {
                    // A loose ratio bound detects a silent switch to storing all input while allowing codec
                    // differences.
                    REQUIRE_LESS(compressed.length().toRawValue(), source.length().toRawValue() * 3U / 4U);
                }
            },
            [&]() {
                return el::text::StringConverter{
                    el::text::StringFormat{"algorithm={} level={} input-bytes={}"_el}.build(
                        algorithm.toString(), static_cast<unsigned>(level), size)}
                    .toStdString();
            });
    }
};
