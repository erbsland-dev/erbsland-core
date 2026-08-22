// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParseError.hpp>
#include <erbsland/mem/ByteCompressionAlgorithm.hpp>
#include <erbsland/mem/ByteCompressionError.hpp>
#include <erbsland/mem/ByteCompressor.hpp>
#include <erbsland/mem/ByteDecompressor.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ByteCompressionAlgorithm ByteCompressionError ByteCompressor ByteDecompressor)
class ByteCompressionTest final : public el::UnitTest {
public:
    void testAlgorithmMetadata() {
        const auto algorithm = el::mem::ByteCompressionAlgorithm{};
        REQUIRE(algorithm == el::mem::ByteCompressionAlgorithm::Lz4Block);
        REQUIRE_EQUAL(algorithm.toString(), "lz4-block"_el);
        REQUIRE(el::mem::ByteCompressionAlgorithm::fromString("lz4-block"_el).has_value());
        REQUIRE_FALSE(el::mem::ByteCompressionAlgorithm::fromString("unknown"_el).has_value());
        REQUIRE_THROWS_AS(el::err::ParseError, el::mem::ByteCompressionAlgorithm::fromStringOrThrow("unknown"_el));
        REQUIRE_EQUAL(el::mem::ByteCompressionAlgorithm::all().size(), std::size_t{1U});
        REQUIRE_GREATER_EQUAL(
            algorithm.maximumCompressedLength(el::unit::ByteLength{1024U}), el::unit::ByteLength{1024U});
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError, algorithm.maximumCompressedLength(el::unit::ByteLength::infinite()));
    }

    void testRawRoundTrips() {
        WITH_CONTEXT(requireRoundTrip({}));
        WITH_CONTEXT(requireRoundTrip({0U}));
        WITH_CONTEXT(requireRoundTrip({1U, 2U, 3U, 4U, 5U}));
        WITH_CONTEXT(requireRoundTrip(std::vector<uint8_t>(300U, 0x41U)));
        auto mixed = std::vector<uint8_t>{};
        for (auto index = uint16_t{}; index < 1024U; ++index) {
            mixed.push_back(static_cast<uint8_t>((index * 37U) & 0xffU));
        }
        WITH_CONTEXT(requireRoundTrip(mixed));
    }

    void testKnownRawBlock() {
        const auto compressed =
            el::mem::ByteBlock({0x40U, 'a', 'b', 'c', 'd', 0x04U, 0x00U, 0x50U, 'a', 'b', 'c', 'd', 'e'});
        const auto expected = el::mem::ByteBlock({'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'e'});
        const auto decompressor = el::mem::ByteDecompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        REQUIRE_EQUAL(decompressor.decompress(compressed, expected.length()), expected);
    }

    void testEnvelopeLayoutAndDispatch() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto compressor = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        const auto envelope = compressor.compressWithEnvelope(source);
        const auto expected = std::vector<uint8_t>{
            'E',
            'L',
            'B',
            'C',
            1U,
            1U,
            0U,
            0U,
            3U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            4U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0U,
            0x30U,
            'a',
            'b',
            'c'};
        REQUIRE_EQUAL(envelope.toUInt8Vector(), expected);
        REQUIRE_EQUAL(el::mem::ByteDecompressor::decompressWithEnvelope(envelope), source);
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            el::mem::ByteDecompressor::decompressWithEnvelope(envelope, el::unit::ByteLength{2U}));
    }

    void testMalformedInput() {
        const auto decompressor = el::mem::ByteDecompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError, decompressor.decompress(el::mem::ByteBlock{}, el::unit::ByteLength{}));
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            decompressor.decompress(el::mem::ByteBlock({0x0fU}), el::unit::ByteLength{}));
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            decompressor.decompress(el::mem::ByteBlock({0x00U}), el::unit::ByteLength{1U}));
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            decompressor.decompress(el::mem::ByteBlock({0x00U, 0x01U, 0x00U}), el::unit::ByteLength{4U}));
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock({'N', 'O', 'P', 'E'})));

        auto envelope = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block}.compressWithEnvelope(
            el::mem::ByteBlock({'x'}));
        auto invalidVersion = el::mem::ByteBlockEditor{envelope};
        invalidVersion.setOrThrow(el::unit::ByteIndex{4U}, el::mem::Byte{2U});
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{invalidVersion}));

        auto invalidAlgorithm = el::mem::ByteBlockEditor{envelope};
        invalidAlgorithm.setOrThrow(el::unit::ByteIndex{5U}, el::mem::Byte{2U});
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{invalidAlgorithm}));

        auto invalidFlags = el::mem::ByteBlockEditor{envelope};
        invalidFlags.setOrThrow(el::unit::ByteIndex{6U}, el::mem::Byte{1U});
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{invalidFlags}));

        auto truncated = el::mem::ByteBlockEditor{envelope};
        truncated.resize(truncated.length() - el::unit::ByteLength::one());
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{truncated}));

        auto trailing = el::mem::ByteBlockEditor{envelope};
        trailing.append(el::mem::Byte{});
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{trailing}));

        auto invalidOriginalLength = el::mem::ByteBlockEditor{envelope};
        invalidOriginalLength.setIntegerOrThrow<uint64_t>(el::unit::ByteIndex{8U}, 2U);
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{invalidOriginalLength}));

        auto invalidPayloadLength = el::mem::ByteBlockEditor{envelope};
        invalidPayloadLength.setIntegerOrThrow<uint64_t>(el::unit::ByteIndex{16U}, 99U);
        REQUIRE_THROWS_AS(
            el::mem::ByteCompressionError,
            el::mem::ByteDecompressor::decompressWithEnvelope(el::mem::ByteBlock{invalidPayloadLength}));
    }

    void testIncrementalState() {
        auto compressor = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        compressor.update(el::mem::ByteBlock({'a', 'b'}));
        compressor.update(el::mem::ByteBlock({'c', 'd'}));
        const auto compressed = compressor.finalize();
        REQUIRE_EQUAL(compressor.finalize(), compressed);
        REQUIRE_THROWS_AS(el::err::LogicError, compressor.finalizeWithEnvelope());
        REQUIRE_THROWS_AS(el::err::LogicError, compressor.update(el::mem::ByteBlock({'x'})));

        auto decompressor = el::mem::ByteDecompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        const auto firstLength = el::unit::ByteLength{compressed.length().toRawValue() / 2U};
        decompressor.update(compressed.slice(el::unit::ByteIndex{}, firstLength));
        decompressor.update(compressed.slice(el::unit::ByteIndex::end(firstLength), compressed.length() - firstLength));
        REQUIRE_EQUAL(decompressor.finalize(el::unit::ByteLength{4U}), el::mem::ByteBlock({'a', 'b', 'c', 'd'}));
        REQUIRE_THROWS_AS(el::err::LogicError, decompressor.finalizeWithEnvelope());

        compressor.reset();
        REQUIRE_FALSE(compressor.isFinalized());
        REQUIRE(compressor.bufferedLength().isZero());
    }

    void testSensitivityPropagation() {
        auto editor = el::mem::ByteBlockEditor({'s', 'e', 'c', 'r', 'e', 't'});
        editor.markAsSensitive();
        const auto sensitive = el::mem::ByteBlock{editor};
        const auto compressor = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        const auto compressed = compressor.compressWithEnvelope(sensitive);
        REQUIRE(compressed.isSensitive());
        REQUIRE(el::mem::ByteDecompressor::decompressWithEnvelope(compressed).isSensitive());
        REQUIRE_FALSE(compressor.compress(sensitive.span()).isSensitive());
    }

    void testIncrementalChunkingEquivalence() {
        const auto source = el::mem::ByteBlock({'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'a', 'b', 'c', 'd', 'e'});
        const auto oneShot =
            el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block}.compressWithEnvelope(source);
        for (auto chunkSize = std::size_t{1U}; chunkSize <= source.length().toSizeT(); ++chunkSize) {
            auto compressor = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
            for (auto offset = std::size_t{}; offset < source.length().toSizeT(); offset += chunkSize) {
                const auto length = std::min(chunkSize, source.length().toSizeT() - offset);
                compressor.update(source.span().subspan(offset, length));
            }
            REQUIRE_EQUAL(compressor.finalizeWithEnvelope(), oneShot);
        }

        auto decompressor = el::mem::ByteDecompressor{};
        decompressor.update(oneShot.span().first(5U));
        decompressor.update(oneShot.span().subspan(5U));
        REQUIRE_EQUAL(decompressor.finalizeWithEnvelope(), source);
        REQUIRE_EQUAL(decompressor.finalizeWithEnvelope(), source);
        decompressor.reset();
        REQUIRE_FALSE(decompressor.isFinalized());
        REQUIRE_THROWS_AS(el::err::LogicError, decompressor.finalize(source.length()));
    }

private:
    void requireRoundTrip(const std::vector<uint8_t> &bytes) {
        const auto source = el::mem::ByteBlock::fromVector(bytes);
        const auto compressor = el::mem::ByteCompressor{el::mem::ByteCompressionAlgorithm::Lz4Block};
        const auto compressed = compressor.compress(source);
        REQUIRE_LESS_EQUAL(compressed.length(), compressor.algorithm().maximumCompressedLength(source.length()));
        const auto decompressor = el::mem::ByteDecompressor{compressor.algorithm()};
        REQUIRE_EQUAL(decompressor.decompress(compressed, source.length()), source);
    }
};
