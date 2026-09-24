// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/compression/ByteCompressor.hpp>
#include <erbsland/compression/ByteDecompressor.hpp>
#include <erbsland/compression/CompressionError.hpp>
#include <erbsland/compression/zip/ArchiveReader.hpp>
#include <erbsland/compression/zip/ArchiveWriter.hpp>
#include <erbsland/compression/zip/ZipError.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/OutOfRangeError.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/stream/ByteBlockInputStream.hpp>
#include <erbsland/stream/TempByteOutputStream.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>

namespace erbsland::test {
/// Generated input whose storage does not grow with its declared length.
/// @notest{Streaming test infrastructure.}
class CompressionGeneratedInput final : public stream::ByteInputStream {
public:
    bool timeout{};
    CompressionGeneratedInput(uint64_t length, bool random) : remaining{length}, random{random} {}
    auto inputSettings() const noexcept -> const stream::InputStreamSettings & override { return settings; }
    auto state() const noexcept -> stream::StreamState override { return stream::StreamState::Open; }
    auto isReady() const noexcept -> bool override { return true; }
    auto waitForReady() -> stream::StreamWaitStatus override { return stream::StreamWaitStatus::Ready; }
    auto close() -> stream::StreamCloseStatus override { return stream::StreamCloseStatus::Closed; }
    void abort() noexcept override {}

protected:
    auto readFromSource(mem::ByteSpan destination, ReadDeadline)
        -> stream::StreamReadResult<unit::ByteLength> override {
        if (timeout) {
            return {stream::StreamReadStatus::Timeout, unit::ByteLength{}};
        }
        const auto count = static_cast<std::size_t>(std::min<uint64_t>(remaining, destination.size()));
        for (std::size_t i{}; i < count; ++i) {
            stateValue ^= stateValue << 13U;
            stateValue ^= stateValue >> 17U;
            stateValue ^= stateValue << 5U;
            destination.data()[i] = mem::Byte::fromCroppedUInt32(random ? stateValue : 65U);
        }
        remaining -= count;
        return {
            count ? stream::StreamReadStatus::Data : stream::StreamReadStatus::Finished,
            unit::ByteLength::fromSizeT(count)};
    }

private:
    stream::InputStreamSettings settings;
    uint64_t remaining;
    bool random;
    uint32_t stateValue{1234567U};
};

/// Collecting or discarding destination recording transfer ownership and write sizes.
/// @notest{Test support for CompressionStreamingTest.}
class CompressionTestOutput final : public stream::ByteOutputStream {
public:
    auto outputSettings() const noexcept -> const stream::OutputStreamSettings & override { return settings; }
    auto state() const noexcept -> stream::StreamState override { return stream::StreamState::Open; }
    auto isReady() const noexcept -> bool override { return true; }
    auto waitForReady() -> stream::StreamWaitStatus override { return stream::StreamWaitStatus::Ready; }
    auto close() -> stream::StreamCloseStatus override {
        ++closeCount;
        return stream::StreamCloseStatus::Closed;
    }
    auto flush() -> stream::StreamWriteStatus override {
        ++flushCount;
        return stream::StreamWriteStatus::Success;
    }
    void abort() noexcept override { ++abortCount; }
    auto write(mem::ConstByteSpan bytes) -> stream::StreamWriteStatus override {
        if (timeout) {
            return stream::StreamWriteStatus::Timeout;
        }
        maximumWrite = std::max(maximumWrite, bytes.size());
        length = length.addedOrThrow(unit::ByteLength::fromSizeT(bytes.size()));
        if (!discard) {
            data.append(bytes);
        }
        return stream::StreamWriteStatus::Success;
    }
    stream::OutputStreamSettings settings;
    mem::ByteBlockEditor data;
    unit::ByteLength length;
    std::size_t maximumWrite{};
    unsigned closeCount{}, flushCount{}, abortCount{};
    bool timeout{}, discard{};
};
}

TESTED_TARGETS(ByteCompressor ByteDecompressor CompressionTransferOptions CompressionTransferResult CompressionProgress)
class CompressionStreamingTest final : public el::UnitTest {
public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testAllFormatsAndLevels() {
        for (const auto algorithm : el::compression::CompressionAlgorithm::all()) {
            for (
                const auto format :
                {el::compression::CompressionFormat::Raw,
                    el::compression::CompressionFormat::Core,
                    el::compression::CompressionFormat::Zip}) {
                if (algorithm == el::compression::CompressionAlgorithm::Lz4Block &&
                    format == el::compression::CompressionFormat::Zip) {
                    continue;
                }
                for (unsigned level{}; level < 5U; ++level) {
                    for (const auto length : {0U, 1U, 170000U}) {
                        WITH_CONTEXT(roundTrip(
                            algorithm, format, static_cast<el::compression::CompressionLevel>(level), length, 71U));
                    }
                }
            }
        }
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testHistoryWindowWrap() {
        for (
            const auto algorithm :
            {el::compression::CompressionAlgorithm::Deflate,
                el::compression::CompressionAlgorithm::Lzma,
                el::compression::CompressionAlgorithm::Zstandard}) {
            WITH_CONTEXT(roundTrip(
                algorithm,
                el::compression::CompressionFormat::Core,
                el::compression::CompressionLevel::Fastest,
                700003U,
                4093U));
        }
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testGeneratedInputExceedsBufferLimit() {
        const auto applicationScope = ApplicationTestScope<>{};
        using namespace el::compression;
        const auto length = el::unit::ByteLength{2U * 1024U * 1024U};
        for (
            const auto algorithm :
            {CompressionAlgorithm::Deflate,
                CompressionAlgorithm::Bzip2,
                CompressionAlgorithm::Lzma,
                CompressionAlgorithm::Zstandard}) {
            for (const auto random : {false, true}) {
                auto source = el::test::CompressionGeneratedInput{length.toRawValue(), random};
                auto file = el::path::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
                const auto transfer =
                    CompressionTransferOptions{}.setMaximumBufferedLength(el::unit::ByteLength{65536U});
                const auto result =
                    ByteCompressor{algorithm, CompressionFormat::Core, CompressionLevel::Fastest}.compress(
                        source, *file, transfer);
                REQUIRE_EQUAL(result.inputLength, length);
                REQUIRE(file->flush() == el::stream::StreamWriteStatus::Success);
                auto encoded = file->path().content().openByteInputStream();
                auto output = el::test::CompressionTestOutput{};
                output.discard = true;
                const auto restored =
                    ByteDecompressor{
                        algorithm, CompressionFormat::Core, DecompressionOptions{}.setExpectedOutputLength(length)}
                        .decompress(*encoded, output, transfer);
                REQUIRE_EQUAL(restored.outputLength, length);
                REQUIRE(output.data.isEmpty());
                REQUIRE_LESS_EQUAL(output.maximumWrite, 65536U);
                REQUIRE(encoded->close().isClosed());
                REQUIRE(file->close().isClosed());
            }
        }
    }
    void testRoundTripLight() {
        WITH_CONTEXT(roundTrip(
            el::compression::CompressionAlgorithm::Deflate,
            el::compression::CompressionFormat::Core,
            el::compression::CompressionLevel::Default,
            1000U,
            1U));
    }
    void testBorrowedStreamsAndExactPrefix() {
        using namespace el::compression;
        const auto data = el::mem::ByteBlock({'a', 'b', 'c', 'd'});
        auto input = el::stream::ByteBlockInputStream{data};
        auto output = el::test::CompressionTestOutput{};
        const auto compressor = ByteCompressor{CompressionAlgorithm::Deflate, CompressionFormat::Raw};
        const auto result = compressor.compress(
            input,
            output,
            CompressionTransferOptions{}
                .setInputLength(el::unit::ByteLength{3U})
                .setBufferLength(el::unit::ByteLength{1U}));
        REQUIRE_EQUAL(result.inputLength, el::unit::ByteLength{3U});
        REQUIRE_EQUAL(input.position(), el::unit::ByteIndex{3U});
        REQUIRE(input.isOpen());
        REQUIRE_EQUAL(output.closeCount, 0U);
        REQUIRE_EQUAL(output.flushCount, 0U);
        REQUIRE_EQUAL(output.abortCount, 0U);
        REQUIRE_LESS_EQUAL(output.maximumWrite, 1U);
        const auto restored =
            ByteDecompressor{CompressionAlgorithm::Deflate, CompressionFormat::Raw}.decompress(output.data);
        REQUIRE_EQUAL(restored, data.slice(el::unit::ByteIndex{}, el::unit::ByteLength{3U}));
    }
    void testCancellationTimeoutAndFallback() {
        using namespace el::compression;
        const auto data = el::mem::ByteBlock({'a', 'b', 'c'});
        auto input = el::stream::ByteBlockInputStream{data};
        auto output = el::test::CompressionTestOutput{};
        const auto compressor = ByteCompressor{CompressionAlgorithm::Deflate, CompressionFormat::Core};
        auto options = CompressionTransferOptions{}.setProgress(
            [](const CompressionProgress &) { return CompressionProgressAction::Cancel; });
        REQUIRE_THROWS_AS(CompressionError, compressor.compress(input, output, options));
        REQUIRE_EQUAL(input.position(), el::unit::ByteIndex{});
        REQUIRE(output.data.isEmpty());
        output.timeout = true;
        REQUIRE_THROWS_AS(CompressionError, compressor.compress(input, output));
        output.timeout = false;
        const auto fallback = ByteCompressor{CompressionAlgorithm::Lz4Block, CompressionFormat::Core};
        REQUIRE_FALSE(fallback.supportsStreaming());
        options = CompressionTransferOptions{}.setFallbackPolicy(CompressionFallbackPolicy::RequireStreaming);
        REQUIRE_THROWS_AS(CompressionError, fallback.compress(input, output, options));
        options = CompressionTransferOptions{}.setMaximumBufferedLength(el::unit::ByteLength{2U});
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, fallback.compress(input, output, options));
    }
    void testReadTimeoutAndOutputLimits() {
        using namespace el::compression;
        auto source = el::test::CompressionGeneratedInput{3U, true};
        source.timeout = true;
        auto output = el::test::CompressionTestOutput{};
        const auto compressor = ByteCompressor{CompressionAlgorithm::Deflate, CompressionFormat::Raw};
        REQUIRE_THROWS_AS(CompressionError, compressor.compress(source, output));
        REQUIRE(output.data.isEmpty());
        auto shortInput = el::stream::ByteBlockInputStream{el::mem::ByteBlock({'a'})};
        REQUIRE_THROWS_AS(
            CompressionError,
            compressor.compress(
                shortInput, output, CompressionTransferOptions{}.setInputLength(el::unit::ByteLength{2U})));
        for (const auto algorithm : {CompressionAlgorithm::Deflate, CompressionAlgorithm::Lz4Block}) {
            const auto compressed =
                ByteCompressor{algorithm, CompressionFormat::Core}.compress(el::mem::ByteBlock({'a', 'b', 'c'}));
            const auto decoder = ByteDecompressor{
                algorithm,
                CompressionFormat::Core,
                DecompressionOptions{}.setMaximumOutputLength(el::unit::ByteLength{2U})};
            REQUIRE_THROWS_AS(el::err::OutOfRangeError, decoder.decompress(compressed));
        }
        auto input = el::stream::ByteBlockInputStream{el::mem::ByteBlock({'a'})};
        const auto limited = ByteCompressor{
            CompressionAlgorithm::Deflate,
            CompressionFormat::Raw,
            CompressionLevel::Default,
            CompressionOptions{}.setMaximumWorkspaceLength(el::unit::ByteLength{1U})};
        REQUIRE_THROWS_AS(el::err::OutOfRangeError, limited.compress(input, output));
        REQUIRE_EQUAL(input.position(), el::unit::ByteIndex{});
    }
    void testZipStreamingLimitsAndZip64() {
        using namespace el::compression;
        using namespace el::compression::zip;
        const auto path = el::path::Path::fromPosix(el::text::String{"stream.bin"});
        for (const auto known : {false, true}) {
            auto destination = std::make_shared<el::test::CompressionTestOutput>();
            auto writer = ArchiveWriter::create(
                destination, ArchiveWriterOptions{}.setMaximumBufferedItemLength(el::unit::ByteLength{16U}));
            writer->setCompressionMethod(CompressionMethod::Stored);
            auto source = std::make_shared<el::test::CompressionGeneratedInput>(4097U, true);
            writer->addStream(source, path, {}, known ? el::unit::ByteLength{4097U} : el::unit::ByteLength::infinite());
            writer->finalize();
            auto reader = ArchiveReader::create(
                destination->data, ArchiveReaderOptions{}.setMaximumBufferedItemLength(el::unit::ByteLength{16U}));
            REQUIRE_EQUAL(reader->isZip64(), !known);
            const auto item = reader->item(el::unit::ItemIndex{});
            REQUIRE_THROWS_AS(ZipError, item->extract(el::unit::ByteLength{10000U}));
            auto output = el::test::CompressionTestOutput{};
            item->extractToStream(output);
            REQUIRE_EQUAL(output.length, el::unit::ByteLength{4097U});
            REQUIRE_EQUAL(output.closeCount, 0U);
            REQUIRE_EQUAL(output.flushCount, 0U);
            auto copied = std::make_shared<el::test::CompressionTestOutput>();
            auto copyWriter = ArchiveWriter::create(copied);
            copyWriter->addItem(*item);
            copyWriter->finalize();
            REQUIRE_EQUAL(
                ArchiveReader::create(copied->data)->item(el::unit::ItemIndex{})->extract(el::unit::ByteLength{10000U}),
                output.data);
        }
        auto destination = std::make_shared<el::test::CompressionTestOutput>();
        auto writer = ArchiveWriter::create(destination, ArchiveWriterOptions{}.setZip64Policy(Zip64Policy::Never));
        REQUIRE_THROWS_AS(
            ZipError, writer->addStream(std::make_shared<el::test::CompressionGeneratedInput>(3U, false), path));
        REQUIRE(destination->data.isEmpty());
    }
    void testZipCancellationAndCallbackExceptions() {
        using namespace el::compression;
        using namespace el::compression::zip;
        const auto path = el::path::Path::fromPosix(el::text::String{"cancel.bin"});
        auto destination = std::make_shared<el::test::CompressionTestOutput>();
        auto writer = ArchiveWriter::create(destination, ArchiveWriterOptions{}.setProgress([](const ZipProgress &) {
            return CompressionProgressAction::Cancel;
        }));
        REQUIRE_THROWS(writer->addData(el::mem::ByteBlock({'a'}), path));
        REQUIRE_EQUAL(destination->abortCount, 1U);
        REQUIRE_THROWS(writer->finalize());
        auto finalDestination = std::make_shared<el::test::CompressionTestOutput>();
        auto finalWriter =
            ArchiveWriter::create(finalDestination, ArchiveWriterOptions{}.setProgress([](const ZipProgress &progress) {
                if (progress.transfer.phase == CompressionPhase::Completed) {
                    throw 42;
                }
                return CompressionProgressAction::Continue;
            }));
        REQUIRE_THROWS_AS(int, finalWriter->addData(el::mem::ByteBlock({'a'}), path));
        REQUIRE_EQUAL(finalDestination->abortCount, 1U);
        REQUIRE_THROWS(finalWriter->finalize());
        auto input = el::stream::ByteBlockInputStream{el::mem::ByteBlock({'a'})};
        auto output = el::test::CompressionTestOutput{};
        auto transfer = CompressionTransferOptions{}.setProgress(
            [](const CompressionProgress &) -> CompressionProgressAction { throw 42; });
        REQUIRE_THROWS_AS(
            int,
            ByteCompressor{CompressionAlgorithm::Deflate, CompressionFormat::Core}.compress(input, output, transfer));
        REQUIRE_EQUAL(input.position(), el::unit::ByteIndex{});
    }
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testTruncationAndTrailingData() {
        using namespace el::compression;
        const auto data = el::mem::ByteBlock({'a', 'b', 'c'});
        for (const auto algorithm : CompressionAlgorithm::all()) {
            const auto encoded = ByteCompressor{algorithm, CompressionFormat::Core}.compress(data);
            const auto decoder = ByteDecompressor{algorithm, CompressionFormat::Core};
            for (std::size_t i{}; i < encoded.length().toSizeT(); ++i) {
                REQUIRE_THROWS_AS(
                    CompressionError,
                    decoder.decompress(encoded.slice(el::unit::ByteIndex{}, el::unit::ByteLength::fromSizeT(i))));
            }
            auto trailing = el::mem::ByteBlockEditor{encoded};
            trailing.append(el::mem::Byte{});
            REQUIRE_THROWS_AS(CompressionError, decoder.decompress(trailing));
        }
    }

private:
    void roundTrip(
        el::compression::CompressionAlgorithm algorithm,
        el::compression::CompressionFormat format,
        el::compression::CompressionLevel level,
        std::size_t size,
        std::size_t chunk) {
        using namespace el::compression;
        auto bytes = el::mem::ByteBlockEditor{};
        for (std::size_t i{}; i < size; ++i) {
            bytes.append(el::mem::Byte::fromCroppedUInt64((i * 37U + i / 1237U) % 251U));
        }
        const auto data = el::mem::ByteBlock{bytes};
        auto input = el::stream::ByteBlockInputStream{data};
        auto encoded = el::test::CompressionTestOutput{};
        auto previous = CompressionTransferResult{};
        unsigned completed{};
        const auto options = CompressionTransferOptions{}
                                 .setBufferLength(el::unit::ByteLength::fromSizeT(chunk))
                                 .setProgress([&](const CompressionProgress &progress) {
                                     REQUIRE_GREATER_EQUAL(progress.transferred.inputLength, previous.inputLength);
                                     REQUIRE_GREATER_EQUAL(progress.transferred.outputLength, previous.outputLength);
                                     previous = progress.transferred;
                                     if (progress.phase == CompressionPhase::Completed) {
                                         ++completed;
                                     }
                                     return CompressionProgressAction::Continue;
                                 });
        const auto compressor = ByteCompressor{algorithm, format, level};
        const auto result = compressor.compress(input, encoded, options);
        REQUIRE_EQUAL(result.inputLength, data.length());
        REQUIRE_EQUAL(completed, 1U);
        REQUIRE_LESS_EQUAL(encoded.length, compressor.maximumCompressedLength(data.length()));
        auto compressedInput = el::stream::ByteBlockInputStream{encoded.data};
        auto decoded = el::test::CompressionTestOutput{};
        auto limits = DecompressionOptions{}.setExpectedOutputLength(data.length());
        const auto decoder = ByteDecompressor{algorithm, format, limits};
        const auto restored = decoder.decompress(
            compressedInput,
            decoded,
            CompressionTransferOptions{}.setBufferLength(el::unit::ByteLength::fromSizeT(chunk)));
        REQUIRE_EQUAL(restored.outputLength, data.length());
        REQUIRE_EQUAL(el::mem::ByteBlock{decoded.data}, data);
    }
};
