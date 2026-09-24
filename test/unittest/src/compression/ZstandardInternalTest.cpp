// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardDecoder.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardEncoder.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardForwardBitReader.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardFseTable.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardHuffmanEncoder.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardHuffmanTable.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardReverseBitReader.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardReverseBitWriter.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardSequenceCode.hpp>
#include <erbsland/compression/impl/zstandard/ZstandardSequenceTable.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <vector>

using namespace el::compression::impl;

TESTED_TARGETS(
    ZstandardForwardBitReader ZstandardReverseBitReader ZstandardReverseBitWriter ZstandardFseEntry ZstandardFseTable
        ZstandardSequenceCode ZstandardSequenceEntry ZstandardSequenceTable ZstandardHuffmanCode ZstandardHuffmanEncoder
            ZstandardHuffmanTable ZstandardDecoder ZstandardMatch ZstandardSequence ZstandardEncoder)
class ZstandardInternalTest final : public el::UnitTest {
public:
    void testReverseBitStream() {
        auto writer = ZstandardReverseBitWriter{};
        writer.append(5U, 3U);
        writer.append(0x1ffU, 9U);
        writer.append(2U, 2U);
        const auto bytes = writer.takeBytes();
        auto reader = ZstandardReverseBitReader{bytes.span(), 0U, bytes.length().toSizeTOrThrow()};
        REQUIRE_EQUAL(reader.read(3U), 5U);
        REQUIRE_EQUAL(reader.read(9U), 0x1ffU);
        REQUIRE_EQUAL(reader.read(2U), 2U);
        REQUIRE(reader.isAtEnd());
    }

    void testPredefinedFseTables() {
        for (
            const auto kind :
            {ZstandardSequenceCode::LiteralLength, ZstandardSequenceCode::Offset, ZstandardSequenceCode::MatchLength}) {
            auto table = ZstandardSequenceTable{};
            table.setPredefined(kind);
            REQUIRE(table.isValid());
            REQUIRE(table.stateCount() >= 32U);
            for (auto state = std::size_t{}; state < table.stateCount(); ++state) {
                static_cast<void>(table.entry(static_cast<uint32_t>(state)));
            }
        }
        auto invalid = ZstandardFseTable{};
        const auto counts = std::array<int16_t, 2U>{1, 1};
        REQUIRE_THROWS_AS(el::compression::CompressionError, invalid.build(counts, 5U));
    }

    void testHuffmanOneAndFourStreams() {
        WITH_CONTEXT(requireHuffmanRoundTrip(600U));
        WITH_CONTEXT(requireHuffmanRoundTrip(4096U));
    }

    void testMalformedEntropyDescriptions() {
        const auto invalidWeights = el::mem::ByteBlock({128U, 0U});
        auto position = std::size_t{};
        auto table = ZstandardHuffmanTable{};
        REQUIRE_THROWS_AS(el::compression::CompressionError, table.read(invalidWeights.span(), position));

        const auto missingRepeatTable = el::mem::ByteBlock({0U, 1U, 0xc0U, 1U});
        auto output = el::mem::ByteBlockEditor{};
        auto decoder = ZstandardDecoder{output, 128U * 1024U, 128U * 1024U};
        REQUIRE_THROWS_AS(el::compression::CompressionError, decoder.decodeBlock(missingRepeatTable.span()));
    }

    void testFseDistributionBounds() {
        using el::compression::CompressionError;
        auto table = ZstandardFseTable{};
        auto lowProbabilities = std::array<int16_t, 33U>{};
        lowProbabilities.fill(-1);
        REQUIRE_THROWS_AS(CompressionError, table.build(lowProbabilities, 5U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 2U>{32, -1}, 5U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 2U>{-2, 34}, 5U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 1U>{31}, 5U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 1U>{33}, 5U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 1U>{8}, 3U));
        REQUIRE_THROWS_AS(CompressionError, table.build(std::array<int16_t, 1U>{1024}, 10U));
        // Every slot can hold a low-probability symbol without decrementing below zero.
        table.build(std::span<const int16_t>{lowProbabilities}.first(32U), 5U);
        for (auto state = uint32_t{}; state < 32U; ++state) {
            REQUIRE_LESS(table.entry(state).symbol, 32U);
            REQUIRE_EQUAL(table.entry(state).bitCount, 5U);
        }
        REQUIRE_THROWS_AS(CompressionError, table.entry(32U));
        table.build(std::array<int16_t, 2U>{31, -1}, 5U);
        for (auto state = uint32_t{}; state < 32U; ++state) {
            REQUIRE_LESS(table.entry(state).symbol, 2U);
        }
    }

    void testBitReaderBounds() {
        using el::compression::CompressionError;
        const auto bytes = el::mem::ByteBlock{0x80U};
        REQUIRE_THROWS_AS(CompressionError, (ZstandardReverseBitReader{bytes.span(), 0U, 2U}));
        REQUIRE_THROWS_AS(CompressionError, (ZstandardReverseBitReader{bytes.span(), 1U, 1U}));
        auto reader = ZstandardReverseBitReader{bytes.span(), 0U, 1U};
        REQUIRE_THROWS_AS(CompressionError, reader.peekPadded(33U));
        REQUIRE_THROWS_AS(CompressionError, reader.discard(8U));
        REQUIRE_EQUAL(reader.read(7U), 0U);
        REQUIRE(reader.isAtEnd());
    }

    void testBlockAndOutputBounds() {
        auto encoder = ZstandardEncoder{{}, el::compression::CompressionLevel::Fastest, 1024U};
        REQUIRE_THROWS_AS(el::err::LogicError, encoder.encodeBlock(1U, 0U));
        REQUIRE_THROWS_AS(el::err::LogicError, encoder.encodeBlock(0U, std::numeric_limits<std::size_t>::max()));
        REQUIRE_THROWS_AS(el::err::LogicError, encoder.setInput({}, 1U));
        const auto history = el::mem::ByteBlock{1U, 2U};
        encoder.setInput(history.span(), 0U);
        REQUIRE_THROWS_AS(el::err::LogicError, encoder.setInput({}, 0U));
        auto output = el::mem::ByteBlockEditor{history};
        auto decoder = ZstandardDecoder{output, 1024U, 1U};
        const auto literals = el::mem::ByteBlock{8U, 'x', 0U};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, decoder.decodeBlock(literals.span()));
        REQUIRE_EQUAL(output.length(), el::unit::ByteLength{2U});
    }

private:
    void requireHuffmanRoundTrip(const std::size_t size) {
        auto literals = std::vector<el::mem::Byte>{};
        literals.reserve(size);
        for (auto index = std::size_t{}; index < size; ++index) {
            literals.emplace_back(static_cast<uint8_t>('a' + (index * 17U + index / 11U) % 23U));
        }
        auto section = ZstandardHuffmanEncoder{literals}.encode();
        REQUIRE(section.has_value());
        section->append(el::mem::Byte{});
        auto output = el::mem::ByteBlockEditor{};
        auto decoder = ZstandardDecoder{output, 128U * 1024U, 128U * 1024U};
        decoder.decodeBlock(section->span());
        REQUIRE_EQUAL(el::mem::ByteBlock{output}, el::mem::ByteBlock::fromSpan(el::mem::ConstByteSpan{literals}));
    }
};
