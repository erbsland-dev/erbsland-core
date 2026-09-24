// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/impl/CodecBitOutput.hpp>
#include <erbsland/compression/impl/CompressionCodec.hpp>
#include <erbsland/compression/impl/lzma/LzmaDecoder.hpp>
#include <erbsland/compression/impl/lzma/LzmaRangeDecoder.hpp>
#include <erbsland/compression/impl/lzma/LzmaRangeEncoder.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/stream/ByteBlockInputStream.hpp>
#include <erbsland/stream/ByteOutputStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <limits>

TESTED_TARGETS(CompressionCodec LzmaRangeDecoder LzmaRangeEncoder)
class CompressionCodecTest final : public el::UnitTest {
private:
    /// Codec test double recording payload dispatch.
    /// @notest{Test support for CompressionCodecTest.}
    class DispatchCodec final : public el::compression::impl::CompressionCodec {
    public:
        DispatchCodec() :
            CompressionCodec{el::compression::CompressionFormat::Raw, el::compression::CompressionLevel::Default} {}

    public: // implement CompressionCodec
        [[nodiscard]] auto algorithm() const noexcept -> el::compression::CompressionAlgorithm override {
            return el::compression::CompressionAlgorithm::Deflate;
        }

    protected: // implement CompressionCodec
        void compressPayload(
            el::compression::impl::CodecReader &input,
            const el::compression::impl::CodecOutput::Write &output,
            const el::compression::CompressionTransferOptions &,
            const el::compression::CompressionOptions &) const override {
            ++compressionCalls;
            consume(input);
            const auto result = el::mem::ByteBlock{'c'};
            output(result.span());
        }
        void decompressPayload(
            el::compression::impl::CodecReader &input,
            const el::compression::impl::CodecOutput::Write &output,
            const el::compression::CompressionTransferOptions &,
            const el::compression::DecompressionOptions &) const override {
            ++decompressionCalls;
            consume(input);
            const auto result = el::mem::ByteBlock{'d'};
            output(result.span());
        }
        [[nodiscard]] auto maximumPayloadLength(const el::unit::ByteLength length) const
            -> el::unit::ByteLength override {
            return length.addedOrThrow(el::unit::ByteLength::one());
        }

    private:
        static void consume(el::compression::impl::CodecReader &input) {
            while (!input.atEnd()) {
                input.take();
            }
        }

    public:
        mutable std::size_t compressionCalls{};
        mutable std::size_t decompressionCalls{};
    };

    /// Collecting byte output stream for codec adapter tests.
    /// @notest{Test support for CompressionCodecTest.}
    class CollectingOutput final : public el::stream::ByteOutputStream {
    public:
        [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
            return settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override {
            return el::stream::StreamState::Open;
        }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto flush() -> el::stream::StreamWriteStatus override { return el::stream::StreamWriteStatus::Success; }
        auto close() -> el::stream::StreamCloseStatus override { return el::stream::StreamCloseStatus::Closed; }
        void abort() noexcept override {}
        auto write(const el::mem::ConstByteSpan value) -> el::stream::StreamWriteStatus override {
            bytes.append(value);
            return el::stream::StreamWriteStatus::Success;
        }

    public:
        el::stream::OutputStreamSettings settings;
        el::mem::ByteBlockEditor bytes;
    };

public:
    void testBitBufferGrowthAndDrain() {
        auto writer = el::mem::BitWriter{};
        writer.writeBits(5U, 3U);
        REQUIRE_FALSE(writer.toByteBlock().isSensitive());
        writer.reserveBytes(131072U);
        REQUIRE_FALSE(writer.toByteBlock().isSensitive());
        auto output = el::mem::ByteBlockEditor{};
        const auto sink = [&](el::mem::ConstByteSpan bytes) { output.append(bytes); };
        el::compression::impl::codecBitOutput::drain(writer, sink);
        REQUIRE(output.isEmpty());
        REQUIRE_EQUAL(writer.bitCount(), 3U);
        REQUIRE_FALSE(writer.toByteBlock().isSensitive());
        writer.writeBits(3U, 5U);
        el::compression::impl::codecBitOutput::drain(writer, sink, true);
        REQUIRE_EQUAL(output, el::mem::ByteBlock({0xa3U}));
        writer.reset();
        writer.writeByte(el::mem::Byte{'a'});
        REQUIRE_FALSE(writer.toByteBlock().isSensitive());
    }

    void testFactoryAndConfiguration() {
        using namespace el::compression;
        for (const auto algorithm : CompressionAlgorithm::all()) {
            for (const auto format : {CompressionFormat::Raw, CompressionFormat::Core}) {
                const auto codec = impl::CompressionCodec::create(algorithm, format, CompressionLevel::High);
                REQUIRE(codec->algorithm() == algorithm);
                REQUIRE(codec->format() == format);
                REQUIRE(codec->level() == CompressionLevel::High);
                const auto shared = codec;
                REQUIRE(shared.get() == codec.get());
            }
        }
    }

    void testPolymorphicPayloadDispatch() {
        auto codec = DispatchCodec{};
        const auto source = el::mem::ByteBlock{'x'};
        REQUIRE_EQUAL(codec.compress(source), el::mem::ByteBlock{'c'});
        REQUIRE_EQUAL(codec.decompress(source, {}), el::mem::ByteBlock{'d'});

        auto callbackOutput = el::mem::ByteBlockEditor{};
        auto callbackUsed = false;
        codec.compress(
            [&]() -> el::mem::ByteBlock {
                if (callbackUsed) {
                    return {};
                }
                callbackUsed = true;
                return source;
            },
            [&](const el::mem::ConstByteSpan bytes) { callbackOutput.append(bytes); },
            {},
            {});
        REQUIRE_EQUAL(callbackOutput, el::mem::ByteBlock{'c'});

        auto streamInput = el::stream::ByteBlockInputStream{source};
        auto streamOutput = CollectingOutput{};
        codec.decompress(streamInput, streamOutput, {}, {});
        REQUIRE_EQUAL(streamOutput.bytes, el::mem::ByteBlock{'d'});
        REQUIRE_EQUAL(codec.compressionCalls, std::size_t{2U});
        REQUIRE_EQUAL(codec.decompressionCalls, std::size_t{2U});
    }

    void testInvalidConfiguration() {
        using namespace el::compression;
        const auto invalidAlgorithm = CompressionAlgorithm{static_cast<CompressionAlgorithm::Value>(0U)};
        REQUIRE_THROWS_AS(
            el::err::ParameterError, impl::CompressionCodec::create(invalidAlgorithm, CompressionFormat::Raw));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            impl::CompressionCodec::create(CompressionAlgorithm::Deflate, static_cast<CompressionFormat>(255U)));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            impl::CompressionCodec::create(
                CompressionAlgorithm::Deflate, CompressionFormat::Raw, static_cast<CompressionLevel>(255U)));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            impl::CompressionCodec::create(CompressionAlgorithm::Lz4Block, CompressionFormat::Zip));
    }

    void testCoreRepresentation() {
        using namespace el::compression;
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto codec = impl::CompressionCodec::create(
            CompressionAlgorithm::Deflate, CompressionFormat::Core, CompressionLevel::Default);
        const auto rawCodec = impl::CompressionCodec::create(
            CompressionAlgorithm::Deflate, CompressionFormat::Raw, CompressionLevel::Default);
        const auto compressed = codec->compress(source);
        REQUIRE(compressed.startsWith({'E', 'L', 'B', 'C', 2U, 2U}));
        REQUIRE_LESS_EQUAL(compressed.length(), codec->maximumCompressedLength(source.length()));
        REQUIRE_EQUAL(
            compressed.slice(el::unit::ByteIndex{12U}, rawCodec->compress(source).length()),
            rawCodec->compress(source));
        REQUIRE_EQUAL(codec->decompress(compressed, {}), source);

        const auto withByte = [&compressed](const std::size_t index, const uint8_t value) {
            auto editor = el::mem::ByteBlockEditor{compressed};
            editor.setOrThrow(el::unit::ByteIndex::fromSizeT(index), el::mem::Byte{value});
            return el::mem::ByteBlock{editor};
        };
        const auto requireRejected = [this, &codec](const el::mem::ByteBlock &invalid) {
            REQUIRE_THROWS_AS(CompressionError, codec->decompress(invalid, {}));
        };
        requireRejected(withByte(4U, 1U));
        requireRejected(withByte(6U, 1U));
        requireRejected(withByte(5U, 0U));
        requireRejected(withByte(5U, 1U));
        requireRejected(withByte(8U, 4U));
        requireRejected(withByte(16U, static_cast<uint8_t>(compressed.span()[16U].toUInt8() + 1U)));

        const auto wrongExpected = DecompressionOptions{}.setExpectedOutputLength(el::unit::ByteLength{4U});
        REQUIRE_THROWS_AS(CompressionError, codec->decompress(compressed, wrongExpected));
        const auto tooSmall = DecompressionOptions{}.setMaximumOutputLength(el::unit::ByteLength{2U});
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, codec->decompress(compressed, tooSmall));
    }

    void testGoldenPayloads() {
        using namespace el::compression;
        const auto source = el::mem::ByteBlock({'a', 'b', 'c'});
        const auto lz4 = impl::CompressionCodec::create(
            CompressionAlgorithm::Lz4Block, CompressionFormat::Raw, CompressionLevel::Default);
        REQUIRE_EQUAL(lz4->compress(source), el::mem::ByteBlock({0x30U, 'a', 'b', 'c'}));

        const auto deflate = impl::CompressionCodec::create(
            CompressionAlgorithm::Deflate, CompressionFormat::Raw, CompressionLevel::Fastest);
        REQUIRE_EQUAL(
            deflate->compress(source), el::mem::ByteBlock({0x01U, 0x03U, 0x00U, 0xfcU, 0xffU, 'a', 'b', 'c'}));
    }

    void testLzmaPositionProperties() {
        using namespace el::compression;
        for (const auto bits : {5U, 32U, 255U}) {
            auto reader = impl::CodecReader{[] { return el::mem::ByteBlock{0U, 0U, 0U, 0U, 0U}; }};
            REQUIRE_THROWS_AS(
                CompressionError,
                (impl::LzmaDecoder{reader, 4096U, 0U, 0U, static_cast<uint8_t>(bits), {}, 1024U, [](auto) {}}));
        }
    }

    void testLzmaProbabilityTreeBounds() {
        auto probabilities = std::array<uint16_t, 1U>{};
        auto encoder = el::compression::impl::LzmaRangeEncoder{};
        REQUIRE_THROWS_AS(el::err::LogicError, encoder.encodeTree(probabilities, 1U, 0U));
        REQUIRE_THROWS_AS(
            el::err::LogicError, encoder.encodeReverseTree(probabilities, std::numeric_limits<uint32_t>::digits, 0U));

        const auto encoded = el::mem::ByteBlock({0U, 0U, 0U, 0U, 0U});
        auto used = false;
        auto reader = el::compression::impl::CodecReader{[&]() -> el::mem::ByteBlock {
            if (used) {
                return {};
            }
            used = true;
            return encoded;
        }};
        auto decoder = el::compression::impl::LzmaRangeDecoder{reader};
        REQUIRE_THROWS_AS(el::err::LogicError, decoder.decodeTree(probabilities, 1U));
        REQUIRE_THROWS_AS(
            el::err::LogicError, decoder.decodeReverseTree(probabilities, std::numeric_limits<uint32_t>::digits));
    }
};
