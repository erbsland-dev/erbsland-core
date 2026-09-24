// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionAlgorithm.hpp>
#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/CompressionFormat.hpp>
#include <erbsland/compression/CompressionLevel.hpp>
#include <erbsland/compression/DecompressionOptions.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(CompressionAlgorithm CompressionError ByteCompressor ByteDecompressor DecompressionOptions)
class ByteCompressionTest final : public el::UnitTest {
public:
    void testAlgorithmMetadata() {
        using Algorithm = el::compression::CompressionAlgorithm;
        REQUIRE(Algorithm{} == Algorithm::Lz4Block);
        const auto expected = std::array{"lz4-block"_el, "deflate"_el, "bzip2"_el, "lzma"_el, "zstandard"_el};
        REQUIRE_EQUAL(Algorithm::all().size(), expected.size());
        for (auto index = std::size_t{}; index < expected.size(); ++index) {
            REQUIRE_EQUAL(Algorithm::all()[index].toString(), expected[index]);
            REQUIRE(Algorithm::fromString(expected[index]).has_value());
        }
        REQUIRE_FALSE(Algorithm::fromString("unknown"_el).has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, Algorithm::fromStringOrThrow("unknown"_el));
    }

    void testOptionsDefaults() {
        const auto options = el::compression::DecompressionOptions{};
        REQUIRE_FALSE(options.expectedOutputLength().has_value());
        REQUIRE_EQUAL(options.maximumOutputLength(), el::unit::ByteLength{256U * 1024U * 1024U});
        REQUIRE_EQUAL(options.maximumWorkspaceLength(), el::unit::ByteLength{64U * 1024U * 1024U});
    }

    void testLz4RoundTrip() {
        WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Lz4Block, {}));
        WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Lz4Block, {0U}));
        WITH_CONTEXT(
            requireRoundTrip(el::compression::CompressionAlgorithm::Lz4Block, std::vector<uint8_t>(300U, 0x41U)));
    }

    void testDeflateRoundTrip() {
        for (const auto level : levels()) {
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Deflate, {}, level));
            WITH_CONTEXT(requireRoundTrip(
                el::compression::CompressionAlgorithm::Deflate, std::vector<uint8_t>(1000U, 0x41U), level));
            auto mixed = std::vector<uint8_t>{};
            for (auto index = uint16_t{}; index < 4096U; ++index) {
                mixed.push_back(static_cast<uint8_t>((index * 37U) & 0xffU));
            }
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Deflate, mixed, level));
        }
    }

    void testKnownDeflateStreams() {
        const auto stored = el::mem::ByteBlock({0x01U, 0x03U, 0x00U, 0xfcU, 0xffU, 'a', 'b', 'c'});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Deflate, stored), el::mem::ByteBlock({'a', 'b', 'c'}));
        const auto fixed = el::mem::ByteBlock({0x4bU, 0x4cU, 0x4aU, 0x06U, 0x00U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Deflate, fixed), el::mem::ByteBlock({'a', 'b', 'c'}));
        const auto dynamic = el::mem::ByteBlock(
            {0xedU,
                0xc1U,
                0x01U,
                0x0dU,
                0x00U,
                0x00U,
                0x00U,
                0xc2U,
                0xa0U,
                0x6cU,
                0xefU,
                0x5fU,
                0xcaU,
                0x1cU,
                0x6eU,
                0x40U,
                0x01U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0x00U,
                0xafU,
                0x06U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Deflate, dynamic).length(),
            el::unit::ByteLength{23'664U});
    }

    void testBzip2RoundTrip() {
        for (const auto level : levels()) {
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Bzip2, {}, level));
            WITH_CONTEXT(requireRoundTrip(
                el::compression::CompressionAlgorithm::Bzip2, std::vector<uint8_t>(1000U, 0x41U), level));
            auto mixed = std::vector<uint8_t>{};
            for (auto index = uint16_t{}; index < 4096U; ++index) {
                mixed.push_back(static_cast<uint8_t>((index * 37U) & 0xffU));
            }
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Bzip2, mixed, level));
        }
        const auto known = el::mem::ByteBlock(
            {0x42U,
                0x5aU,
                0x68U,
                0x39U,
                0x31U,
                0x41U,
                0x59U,
                0x26U,
                0x53U,
                0x59U,
                0x64U,
                0x8cU,
                0xbbU,
                0x73U,
                0x00U,
                0x00U,
                0x00U,
                0x01U,
                0x00U,
                0x38U,
                0x00U,
                0x20U,
                0x00U,
                0x21U,
                0x98U,
                0x19U,
                0x84U,
                0x61U,
                0x77U,
                0x24U,
                0x53U,
                0x85U,
                0x09U,
                0x06U,
                0x48U,
                0xcbU,
                0xb7U,
                0x30U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Bzip2, known), el::mem::ByteBlock({'a', 'b', 'c'}));
    }

    void testLzmaRoundTrip() {
        for (const auto level : levels()) {
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Lzma, {}, level));
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Lzma, {0x41U}, level));
            WITH_CONTEXT(
                requireRoundTrip(el::compression::CompressionAlgorithm::Lzma, std::vector<uint8_t>(10U, 0x41U), level));
            WITH_CONTEXT(requireRoundTrip(
                el::compression::CompressionAlgorithm::Lzma, std::vector<uint8_t>(1000U, 0x41U), level));
        }
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto compressed =
            el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Lzma, el::compression::CompressionFormat::Zip}
                .compress(source);
        const auto decompressor = el::compression::ByteDecompressor{
            el::compression::CompressionAlgorithm::Lzma, el::compression::CompressionFormat::Zip};
        REQUIRE_EQUAL(decompressor.decompress(compressed), source);
        const auto known = el::mem::ByteBlock(
            {0x5dU,
                0x00U,
                0x00U,
                0x80U,
                0x00U,
                0xffU,
                0xffU,
                0xffU,
                0xffU,
                0xffU,
                0xffU,
                0xffU,
                0xffU,
                0x00U,
                0x30U,
                0x98U,
                0x88U,
                0xa4U,
                0x4aU,
                0x8eU,
                0x9fU,
                0xffU,
                0xf6U,
                0x63U,
                0x80U,
                0x00U});
        REQUIRE_EQUAL(decompress(el::compression::CompressionAlgorithm::Lzma, known), source);
        const auto knownWithExpectedLength =
            el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Lzma,
                el::compression::CompressionFormat::Raw,
                el::compression::DecompressionOptions{}.setExpectedOutputLength(source.length())}
                .decompress(known);
        REQUIRE_EQUAL(knownWithExpectedLength, source);

        auto repeated = std::vector<uint8_t>{};
        repeated.reserve(16U * 1024U);
        for (auto index = std::size_t{}; index < 16U * 1024U; ++index) {
            repeated.push_back(static_cast<uint8_t>('a' + index % 5U));
        }
        const auto repeatedSource = el::mem::ByteBlock::fromVector(repeated);
        const auto repeatedCompressed =
            el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Lzma, el::compression::CompressionFormat::Zip}
                .compress(repeatedSource);
        const auto repeatedDecompressed =
            el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Lzma, el::compression::CompressionFormat::Zip}
                .decompress(repeatedCompressed);
        REQUIRE_EQUAL(repeatedDecompressed, repeatedSource);
    }

    void testZstandardRoundTrip() {
        for (const auto level : levels()) {
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Zstandard, {}, level));
            WITH_CONTEXT(requireRoundTrip(
                el::compression::CompressionAlgorithm::Zstandard, std::vector<uint8_t>(1000U, 0x41U), level));
            auto mixed = std::vector<uint8_t>{};
            for (auto index = uint16_t{}; index < 4096U; ++index) {
                mixed.push_back(static_cast<uint8_t>((index * 37U) & 0xffU));
            }
            WITH_CONTEXT(requireRoundTrip(el::compression::CompressionAlgorithm::Zstandard, mixed, level));
        }
        const auto known =
            el::mem::ByteBlock({0x28U, 0xb5U, 0x2fU, 0xfdU, 0x20U, 0x03U, 0x19U, 0x00U, 0x00U, 0x61U, 0x62U, 0x63U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Zstandard, known), el::mem::ByteBlock({'a', 'b', 'c'}));
        const auto zeroDictionaryId = el::mem::ByteBlock(
            {0x28U, 0xb5U, 0x2fU, 0xfdU, 0x21U, 0x00U, 0x03U, 0x19U, 0x00U, 0x00U, 0x61U, 0x62U, 0x63U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Zstandard, zeroDictionaryId),
            el::mem::ByteBlock({'a', 'b', 'c'}));

        auto repetitive = std::vector<uint8_t>{};
        for (auto repeat = std::size_t{}; repeat < 400U; ++repeat) {
            for (const auto byte : std::array<uint8_t, 8U>{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'}) {
                repetitive.push_back(byte);
            }
        }
        const auto repetitiveSource = el::mem::ByteBlock::fromVector(repetitive);
        for (const auto level : levels()) {
            const auto compressed =
                el::compression::ByteCompressor{
                    el::compression::CompressionAlgorithm::Zstandard, el::compression::CompressionFormat::Raw, level}
                    .compress(repetitiveSource);
            REQUIRE_LESS(compressed.length(), repetitiveSource.length());
            REQUIRE_EQUAL(decompress(el::compression::CompressionAlgorithm::Zstandard, compressed), repetitiveSource);
        }
    }

    void testFormatMatrix() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c', 'a', 'b', 'c'});
        for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
            for (
                const auto format :
                {el::compression::CompressionFormat::Raw, el::compression::CompressionFormat::Core}) {
                auto options = el::compression::DecompressionOptions{}.setExpectedOutputLength(source.length());
                const auto compressor = el::compression::ByteCompressor{algorithm, format};
                const auto compressed = compressor.compress(source);
                REQUIRE_LESS_EQUAL(compressed.length(), compressor.maximumCompressedLength(source.length()));
                const auto decompressor = el::compression::ByteDecompressor{algorithm, format, options};
                REQUIRE_EQUAL(decompressor.decompress(compressed), source);
            }
            if (algorithm != el::compression::CompressionAlgorithm::Lz4Block) {
                const auto compressor =
                    el::compression::ByteCompressor{algorithm, el::compression::CompressionFormat::Zip};
                const auto compressed = compressor.compress(source);
                const auto decompressor =
                    el::compression::ByteDecompressor{algorithm, el::compression::CompressionFormat::Zip};
                REQUIRE_EQUAL(decompressor.decompress(compressed), source);
            }
        }
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            (el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Lz4Block, el::compression::CompressionFormat::Zip}));
    }

    void testCoreEnvelopeAndMismatch() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto compressor = el::compression::ByteCompressor{
            el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Core};
        const auto envelope = compressor.compress(source);
        REQUIRE(envelope.startsWith({'E', 'L', 'B', 'C', 2U, 2U}));
        const auto decompressor = el::compression::ByteDecompressor{
            el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Core};
        REQUIRE_EQUAL(decompressor.decompress(envelope), source);
        const auto mismatch = el::compression::ByteDecompressor{
            el::compression::CompressionAlgorithm::Lz4Block, el::compression::CompressionFormat::Core};
        REQUIRE_THROWS_AS(el::compression::CompressionError, mismatch.decompress(envelope));
    }

    void testLimitsAndValidation() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto compressed =
            el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Raw}
                .compress(source);
        auto options = el::compression::DecompressionOptions{}.setMaximumOutputLength(el::unit::ByteLength{2U});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Raw, options}
                .decompress(compressed));
        options = el::compression::DecompressionOptions{}.setExpectedOutputLength(el::unit::ByteLength{4U});
        REQUIRE_THROWS_AS(
            el::compression::CompressionError,
            el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Raw, options}
                .decompress(compressed));
        options = el::compression::DecompressionOptions{}.setMaximumWorkspaceLength(el::unit::ByteLength{1U});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Raw, options}
                .decompress(compressed));
    }

    void testMalformedAndWorkspacePolicies() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto deflate =
            el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Raw}
                .compress(source);
        auto trailingEditor = el::mem::ByteBlockEditor{deflate};
        trailingEditor.append(el::mem::Byte{0U});
        REQUIRE_THROWS_AS(
            el::compression::CompressionError,
            decompress(el::compression::CompressionAlgorithm::Deflate, el::mem::ByteBlock{trailingEditor}));

        for (
            const auto algorithm :
            {el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionAlgorithm::Bzip2}) {
            const auto compressed =
                el::compression::ByteCompressor{algorithm, el::compression::CompressionFormat::Raw}.compress(source);
            const auto truncated =
                compressed.slice(el::unit::ByteIndex{0U}, compressed.length() - el::unit::ByteLength{1U});
            WITH_CONTEXT(requireMalformed(algorithm, truncated));
        }

        const auto entropyZstandard = el::mem::ByteBlock(
            {0x28U,
                0xb5U,
                0x2fU,
                0xfdU,
                0x60U,
                0xe8U,
                0x02U,
                0x4dU,
                0x00U,
                0x00U,
                0x10U,
                0x41U,
                0x41U,
                0x01U,
                0x00U,
                0xe3U,
                0x2bU,
                0x80U,
                0x05U});
        REQUIRE_EQUAL(
            decompress(el::compression::CompressionAlgorithm::Zstandard, entropyZstandard),
            el::mem::ByteBlock(el::unit::ByteLength{1000U}, el::mem::Byte{'A'}));

        const auto lzma =
            el::compression::ByteCompressor{
                el::compression::CompressionAlgorithm::Lzma,
                el::compression::CompressionFormat::Raw,
                el::compression::CompressionLevel::Fastest}
                .compress(source);
        const auto options =
            el::compression::DecompressionOptions{}.setMaximumWorkspaceLength(el::unit::ByteLength{256U * 1024U});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            (el::compression::ByteDecompressor{
                el::compression::CompressionAlgorithm::Lzma, el::compression::CompressionFormat::Raw, options}
                    .decompress(lzma)));
    }

    void testReusableConfigurationAndSensitivity() {
        auto compressor = el::compression::ByteCompressor{
            el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Core};
        const auto source = el::mem::ByteBlock({'a', 'b', 'c', 'd'});
        const auto compressed = compressor.compress(source);
        const auto decompressor = el::compression::ByteDecompressor{
            el::compression::CompressionAlgorithm::Deflate, el::compression::CompressionFormat::Core};
        REQUIRE_EQUAL(decompressor.decompress(compressed), source);
        REQUIRE_EQUAL(compressor.compress(source), compressed);

        auto editor = el::mem::ByteBlockEditor({'s', 'e', 'c', 'r', 'e', 't'});
        editor.markAsSensitive();
        const auto sensitive = el::mem::ByteBlock{editor};
        const auto encoded = compressor.compress(sensitive);
        REQUIRE_FALSE(encoded.isSensitive());
        REQUIRE_FALSE(decompressor.decompress(encoded).isSensitive());
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testSensitiveInputsAreOrdinaryCodecData() {
        using namespace el::compression;
        auto source = el::mem::ByteBlock{'a', 'b', 'c', 'd'};
        source.markAsSensitive();
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto format : {CompressionFormat::Raw, CompressionFormat::Zip, CompressionFormat::Core}) {
                if (algorithm == CompressionAlgorithm::Lz4Block && format == CompressionFormat::Zip) {
                    continue;
                }
                const auto compressor = ByteCompressor{algorithm, format};
                auto compressed = compressor.compress(source);
                REQUIRE_FALSE(compressed.isSensitive());
                compressed.markAsSensitive();
                const auto options = DecompressionOptions{}.setExpectedOutputLength(source.length());
                const auto decoded = ByteDecompressor{algorithm, format, options}.decompress(compressed);
                REQUIRE_EQUAL(decoded, source);
                REQUIRE_FALSE(decoded.isSensitive());
                REQUIRE(compressed.isSensitive());
                REQUIRE(source.isSensitive());
            }
        }
    }

private:
    [[nodiscard]] static auto levels() -> std::array<el::compression::CompressionLevel, 5U> {
        using Level = el::compression::CompressionLevel;
        return {Level::Fastest, Level::Fast, Level::Default, Level::High, Level::Highest};
    }
    [[nodiscard]] static auto decompress(
        const el::compression::CompressionAlgorithm algorithm, const el::mem::ByteBlock &data) -> el::mem::ByteBlock {
        return el::compression::ByteDecompressor{algorithm, el::compression::CompressionFormat::Raw}.decompress(data);
    }
    void requireMalformed(const el::compression::CompressionAlgorithm algorithm, const el::mem::ByteBlock &compressed) {
        auto thrown = false;
        try {
            static_cast<void>(decompress(algorithm, compressed));
        } catch (const el::compression::CompressionError &error) {
            thrown = true;
            REQUIRE_EQUAL(error.reasonCode(), el::compression::CompressionErrorReason::MalformedData);
        }
        REQUIRE(thrown);
    }
    void requireRoundTrip(
        const el::compression::CompressionAlgorithm algorithm,
        const std::vector<uint8_t> &bytes,
        const el::compression::CompressionLevel level = el::compression::CompressionLevel::Default) {
        const auto source = el::mem::ByteBlock::fromVector(bytes);
        const auto compressor =
            el::compression::ByteCompressor{algorithm, el::compression::CompressionFormat::Raw, level};
        const auto compressed = compressor.compress(source);
        REQUIRE_LESS_EQUAL(compressed.length(), compressor.maximumCompressedLength(source.length()));
        auto options = el::compression::DecompressionOptions{}.setExpectedOutputLength(source.length());
        const auto decompressor =
            el::compression::ByteDecompressor{algorithm, el::compression::CompressionFormat::Raw, options};
        REQUIRE_EQUAL(decompressor.decompress(compressed), source);
    }
};
