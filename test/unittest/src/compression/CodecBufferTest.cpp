// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/impl/CodecBitReader.hpp>
#include <erbsland/compression/impl/CodecOutput.hpp>
#include <erbsland/compression/impl/CodecReader.hpp>
#include <erbsland/compression/impl/CompressionCrc32.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <utility>

TESTED_TARGETS(CodecBitReader CodecOutput CodecReader CompressionCrc32)
class CodecBufferTest final : public el::UnitTest {
public:
    void testBitWidthAndTruncation() {
        using namespace el::compression;
        auto pending = el::mem::ByteBlock{0xa5U};
        auto source = impl::CodecReader{[&] { return std::exchange(pending, el::mem::ByteBlock{}); }};
        auto reader = impl::CodecBitReader{source};
        REQUIRE_EQUAL(reader.readBits(0U), 0U);
        REQUIRE_THROWS_AS(CompressionError, reader.readBits(65U));
        REQUIRE_THROWS_AS(CompressionError, reader.readBits(9U));
        REQUIRE_EQUAL(reader.readBits(8U), 0xa5U);
        REQUIRE_THROWS_AS(CompressionError, reader.readBits(1U));
    }

    void testIntegerWidthLimit() {
        auto reader = el::compression::impl::CodecReader{[] { return el::mem::ByteBlock{}; }};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, reader.little(9U));
    }

    void testIncrementalCrc32() {
        namespace m = el::mem;
        auto data = m::ByteBlockEditor{};
        uint32_t expected{0xffffffffU};
        auto crc = el::compression::impl::CompressionCrc32{};
        REQUIRE_EQUAL(crc.value(), 0U);
        for (uint32_t value{}; value < 256U; ++value) {
            data.append(m::Byte{static_cast<uint8_t>(value)});
            expected ^= value;
            for (unsigned bit{}; bit < 8U; ++bit) {
                expected = (expected >> 1U) ^ (0xedb88320U & (0U - (expected & 1U)));
            }
        }
        for (std::size_t position{}; position < data.span().size();) {
            const auto count = std::min(std::size_t{7U}, data.span().size() - position);
            crc.update(data.span().subspan(position, count));
            position += count;
        }
        REQUIRE_EQUAL(crc.value(), ~expected);
        const auto known = m::ByteBlock{'1', '2', '3', '4', '5', '6', '7', '8', '9'};
        auto knownCrc = el::compression::impl::CompressionCrc32{};
        knownCrc.update(known.span());
        REQUIRE_EQUAL(knownCrc.value(), uint32_t{0xcbf43926U});
    }

    void testOverlappingMatchesAndHistoryWrap() {
        namespace m = el::mem;
        namespace u = el::unit;
        for (const auto window : {1U, 7U, 32U, 32768U}) {
            auto actual = m::ByteBlockEditor{};
            auto expected = m::ByteBlockEditor{};
            auto output = el::compression::impl::CodecOutput{
                [&](m::ConstByteSpan bytes) {
                    REQUIRE_LESS_EQUAL(bytes.size(), 65536U);
                    actual.append(bytes);
                },
                window,
                u::ByteLength{200000U}};
            output.append(m::Byte{0x91U}, u::ByteLength{65533U});
            expected.append(m::Byte{0x91U}, u::ByteLength{65533U});
            uint32_t random{123456789U};
            for (unsigned operation{}; operation < 64U; ++operation) {
                random = random * 1664525U + 1013904223U;
                const auto distance = uint64_t{random % window + 1U};
                const auto count = uint64_t{random % 257U + 1U};
                const auto start = expected.length().toRawValue() - distance;
                output.appendRepeated(u::ByteRange{u::ByteIndex{start}, u::ByteLength{distance}}, u::ByteLength{count});
                for (uint64_t i{}; i < count; ++i) {
                    expected.append(expected.get(u::ByteIndex{start + i % distance}));
                }
                const auto literal = m::Byte{static_cast<uint8_t>(random >> 24U)};
                output.append(literal);
                expected.append(literal);
                if (operation % 7U == 0U) {
                    output.flush();
                }
            }
            output.flush();
            REQUIRE_EQUAL(output.length(), expected.length());
            REQUIRE_EQUAL(actual, expected);
        }
    }

    void testSpanHistoryAndLimitsBeforeEmission() {
        namespace m = el::mem;
        namespace u = el::unit;
        const auto input = m::ByteBlock{0x01U, 0x23U, 0x45U, 0x67U, 0x89U};
        auto actual = m::ByteBlockEditor{};
        auto output = el::compression::impl::CodecOutput{
            [&](m::ConstByteSpan bytes) { actual.append(bytes); }, 3U, u::ByteLength{10U}};
        output.append(input.span());
        output.flush();
        REQUIRE_EQUAL(output.getOrThrow(u::ByteIndex{2U}), m::Byte{0x45U});
        REQUIRE_THROWS_AS(el::err::LogicError, output.getOrThrow(u::ByteIndex{1U}));
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, output.append(m::Byte{}, u::ByteLength{6U}));
        REQUIRE_THROWS_AS(
            el::err::OutOfRangeError,
            output.appendRepeated(u::ByteRange{u::ByteIndex{2U}, u::ByteLength{3U}}, u::ByteLength{6U}));
        REQUIRE_EQUAL(output.length(), u::ByteLength{5U});
        REQUIRE_EQUAL(actual, input);
        output.appendRepeated(u::ByteRange{u::ByteIndex{2U}, u::ByteLength{3U}}, u::ByteLength{5U});
        output.flush();
        REQUIRE_EQUAL(actual, m::ByteBlock({1U, 0x23U, 0x45U, 0x67U, 0x89U, 0x45U, 0x67U, 0x89U, 0x45U, 0x67U}));
    }

    void testReaderSlicesAndAssemblesChunks() {
        namespace m = el::mem;
        namespace u = el::unit;
        auto input = m::ByteBlock{1U, 2U, 3U, 4U, 5U};
        input.markAsSensitive();
        unsigned reads{};
        auto reader = el::compression::impl::CodecReader{[&]() -> m::ByteBlock {
            ++reads;
            return reads <= 2U ? input : m::ByteBlock{};
        }};
        const auto first = reader.block(3U);
        REQUIRE_EQUAL(first, m::ByteBlock({1U, 2U, 3U}));
        REQUIRE(first.isSensitive());
        REQUIRE(first.span().data() == input.span().data());
        const auto joined = reader.block(4U);
        REQUIRE_EQUAL(joined, m::ByteBlock({4U, 5U, 1U, 2U}));
        REQUIRE_FALSE(joined.isSensitive());
        REQUIRE_EQUAL(reader.take(), m::ByteBlock({3U, 4U, 5U}));
        REQUIRE(reader.atEnd());
        REQUIRE(reader.block(16U).isEmpty());
        REQUIRE_EQUAL(reads, 3U);
    }
};
