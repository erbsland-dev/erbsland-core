// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "ZipTestStreams.hpp"

#include "../core/ApplicationTestScope.hpp"

#include <erbsland/compression/zip/ArchiveItem.hpp>
#include <erbsland/compression/zip/ArchiveReader.hpp>
#include <erbsland/compression/zip/ArchiveWriter.hpp>
#include <erbsland/compression/zip/CompressionMethod.hpp>
#include <erbsland/compression/zip/impl/ZipTools.hpp>
#include <erbsland/compression/zip/Zip64Policy.hpp>
#include <erbsland/compression/zip/ZipError.hpp>
#include <erbsland/err/LogicError.hpp>
#include <erbsland/err/ParameterError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/mem/ByteBlockEditor.hpp>
#include <erbsland/mem/ByteWriter.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathInfo.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/stream/ByteInputStream.hpp>
#include <erbsland/stream/InputStreamSettings.hpp>
#include <erbsland/stream/StreamCloseStatus.hpp>
#include <erbsland/stream/StreamPositionOrigin.hpp>
#include <erbsland/stream/StreamPositionStatus.hpp>
#include <erbsland/stream/StreamReadStatus.hpp>
#include <erbsland/stream/StreamState.hpp>
#include <erbsland/stream/StreamWaitStatus.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <limits>
#include <system_error>

using namespace el::text::literals;

TESTED_TARGETS(
    ArchiveReader ArchiveItem ArchiveWriter ArchiveReaderOptions ArchiveWriterOptions ArchiveEntryOptions
        ArchiveExtractionOptions ArchiveDirectoryOptions ZipError CompressionMethod Zip64Policy globMatches
            normalizeEntryPath modificationTimeFromExtra appendExtendedTimestamp)
class ZipArchiveTest final : public el::UnitTest {
    using TrackingInputStream = erbsland::test::ZipTrackingInputStream;

private:
    [[nodiscard]] static auto makeDescriptorArchive(const bool signedDescriptor) -> el::mem::ByteBlock {
        namespace tools = el::compression::zip::impl::zipTools;
        constexpr auto cFlags = uint16_t{tools::cUtf8Flag | tools::cDescriptorFlag};
        constexpr auto cDosDate = uint16_t{0x5821U};
        constexpr auto cCrc32 = uint32_t{0x352441c2U};
        auto result = el::mem::ByteWriter{};

        result.writeUInt32(tools::cLocalHeaderSignature)
            .writeUInt16(20U)
            .writeUInt16(cFlags)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(cDosDate)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt16(1U)
            .writeUInt16(0U)
            .writeByte(el::mem::Byte{'x'})
            .writeBytes(el::mem::ByteBlock({'a', 'b', 'c'}));
        if (signedDescriptor) {
            result.writeUInt32(tools::cDataDescriptorSignature);
        }
        result.writeUInt32(cCrc32).writeUInt32(3U).writeUInt32(3U);
        const auto directoryOffset = result.length().toRawValue();

        result.writeUInt32(tools::cCentralHeaderSignature)
            .writeUInt16(static_cast<uint16_t>((3U << 8U) | 20U))
            .writeUInt16(20U)
            .writeUInt16(cFlags)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(cDosDate)
            .writeUInt32(cCrc32)
            .writeUInt32(3U)
            .writeUInt32(3U)
            .writeUInt16(1U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt32(static_cast<uint32_t>(0100644U << 16U))
            .writeUInt32(0U)
            .writeByte(el::mem::Byte{'x'});
        const auto directoryLength = result.length().toRawValue() - directoryOffset;

        result.writeUInt32(tools::cEndSignature)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(1U)
            .writeUInt16(1U)
            .writeUInt32(static_cast<uint32_t>(directoryLength))
            .writeUInt32(static_cast<uint32_t>(directoryOffset))
            .writeUInt16(0U);
        return result.toByteBlock();
    }

    void requireOpenReason(const el::mem::ByteBlock &data, const el::compression::zip::ZipErrorReason expectedReason) {
        auto rejected = false;
        try {
            static_cast<void>(el::compression::zip::ArchiveReader::create(data));
        } catch (const el::compression::zip::ZipError &error) {
            rejected = true;
            REQUIRE_EQUAL(error.reasonCode(), expectedReason);
        }
        REQUIRE(rejected);
    }

public:
    void testEmptyArchive() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "empty.zip"_el;
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->finalize();
        writer->finalize();
        REQUIRE(writer->isFinalized());
        REQUIRE(archivePath.info().isRegularFile());

        auto reader = el::compression::zip::ArchiveReader::create(archivePath);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{});
        REQUIRE_FALSE(reader->isZip64());
        REQUIRE(reader->comment().isEmpty());
        reader->close();
        reader->close();
        REQUIRE_FALSE(reader->isOpen());
    }

    void testAllCompressionMethodsRoundTrip() {
        const auto applicationScope = ApplicationTestScope<>{};
        constexpr auto methods = std::array{
            el::compression::zip::CompressionMethod::Stored,
            el::compression::zip::CompressionMethod::Deflate,
            el::compression::zip::CompressionMethod::Bzip2,
            el::compression::zip::CompressionMethod::Lzma,
            el::compression::zip::CompressionMethod::Zstandard};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "methods.zip"_el;
        const auto expected = el::mem::ByteBlock({'a', 'b', 'c', 'a', 'b', 'c', 'a', 'b', 'c'});
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->setComment("archive comment"_el);
        for (auto index = std::size_t{}; index < methods.size(); ++index) {
            auto options = el::compression::zip::ArchiveEntryOptions{};
            options.setCompressionMethod(methods[index]).setComment("item comment"_el);
            writer->addData(
                expected,
                el::path::Path::fromPosix(
                    el::text::String{"item"_el}.inserted(el::unit::CpIndex{4U}, el::text::String::fromInteger(index))),
                options);
        }
        writer->finalize();

        auto reader = el::compression::zip::ArchiveReader::create(archivePath);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{methods.size()});
        REQUIRE_EQUAL(reader->comment(), "archive comment"_el);
        for (auto index = std::size_t{}; index < methods.size(); ++index) {
            const auto item = reader->item(el::unit::ItemIndex::fromSizeT(index));
            REQUIRE(item != nullptr);
            REQUIRE_EQUAL(item->compressionMethod(), methods[index]);
            REQUIRE_EQUAL(item->comment(), "item comment"_el);
            REQUIRE_EQUAL(item->extract(el::unit::ByteLength{1024U}), expected);
        }
    }

    void testForcedZip64AndRawCopy() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto firstPath = temporary->path() / "first.zip"_el;
        const auto secondPath = temporary->path() / "second.zip"_el;
        auto writerOptions = el::compression::zip::ArchiveWriterOptions{};
        writerOptions.setZip64Policy(el::compression::zip::Zip64Policy::Always);
        auto writer = el::compression::zip::ArchiveWriter::create(firstPath, writerOptions);
        auto entryOptions = el::compression::zip::ArchiveEntryOptions{};
        entryOptions.setCompressionMethod(el::compression::zip::CompressionMethod::Zstandard);
        writer->addData(
            el::mem::ByteBlock({'c', 'o', 'p', 'y'}), el::path::Path::fromPosix("data.bin"_el), entryOptions);
        writer->finalize();

        auto reader = el::compression::zip::ArchiveReader::create(firstPath);
        REQUIRE(reader->isZip64());
        auto copiedWriter = el::compression::zip::ArchiveWriter::create(secondPath);
        copiedWriter->addItem(*reader->item(el::unit::ItemIndex{0U}));
        copiedWriter->finalize();
        auto copiedReader = el::compression::zip::ArchiveReader::create(secondPath);
        REQUIRE_EQUAL(
            copiedReader->item(el::unit::ItemIndex{0U})->extract(el::unit::ByteLength{16U}),
            el::mem::ByteBlock({'c', 'o', 'p', 'y'}));
    }

    void testCrcFailureAndClosedItemLifetime() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "crc.zip"_el;
        auto options = el::compression::zip::ArchiveEntryOptions{};
        options.setCompressionMethod(el::compression::zip::CompressionMethod::Stored);
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->addData(el::mem::ByteBlock({'a', 'b', 'c'}), el::path::Path::fromPosix("x"_el), options);
        writer->finalize();
        auto bytes = el::mem::ByteBlockEditor{archivePath.content().readDataOrThrow()};
        const auto nameLength = bytes.getInteger<uint16_t>(el::unit::ByteIndex{26U});
        const auto extraLength = bytes.getInteger<uint16_t>(el::unit::ByteIndex{28U});
        const auto payloadOffset = el::unit::ByteIndex{static_cast<uint64_t>(30U + nameLength + extraLength)};
        bytes.set(payloadOffset, el::mem::Byte{'z'});
        const auto corruptPath = temporary->path() / "corrupt.zip"_el;
        corruptPath.content().writeDataOrThrow(bytes);
        auto reader = el::compression::zip::ArchiveReader::create(corruptPath);
        const auto item = reader->item(el::unit::ItemIndex{0U});
        REQUIRE_THROWS_AS(el::compression::zip::ZipError, item->extract(el::unit::ByteLength{16U}));
        reader->close();
        REQUIRE_EQUAL(item->path(), el::path::Path::fromPosix("x"_el));
        REQUIRE_THROWS_AS(el::err::LogicError, item->extract(el::unit::ByteLength{16U}));
    }

    void testOwnedStreamsAndPositioningRequirement() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "stream.zip"_el;
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        const auto itemSource =
            std::make_shared<TrackingInputStream>(el::mem::ByteBlock({'a', 'b', 'c', 'd', 'e'}), false);
        writer->addStream(itemSource, el::path::Path::fromPosix("part.bin"_el), {}, el::unit::ByteLength{3U});
        REQUIRE_EQUAL(itemSource->bytesRead, std::size_t{3U});
        REQUIRE_EQUAL(itemSource->closeCount, std::size_t{1U});
        REQUIRE_EQUAL(itemSource->abortCount, std::size_t{});
        writer->finalize();

        const auto archiveData = archivePath.content().readDataOrThrow();
        const auto nonPositionable = std::make_shared<TrackingInputStream>(archiveData, false);
        REQUIRE_THROWS_AS(el::err::ParameterError, el::compression::zip::ArchiveReader::create(nonPositionable));
        REQUIRE_EQUAL(nonPositionable->bytesRead, std::size_t{});
        REQUIRE_EQUAL(nonPositionable->closeCount, std::size_t{1U});
        REQUIRE_EQUAL(nonPositionable->abortCount, std::size_t{});
    }

    void testSeekableInputStaysLazy() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "lazy.zip"_el;
        const auto payload = el::mem::ByteBlock(el::unit::ByteLength{1024U * 1024U}, el::mem::Byte{'x'});
        auto entryOptions = el::compression::zip::ArchiveEntryOptions{};
        entryOptions.setCompressionMethod(el::compression::zip::CompressionMethod::Stored);
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->addData(payload, el::path::Path::fromPosix("first.bin"_el), entryOptions);
        writer->addData(payload, el::path::Path::fromPosix("selected.bin"_el), entryOptions);
        writer->finalize();

        const auto archiveData = archivePath.content().readDataOrThrow();
        const auto source = std::make_shared<TrackingInputStream>(archiveData, true);
        auto reader = el::compression::zip::ArchiveReader::create(source);
        REQUIRE(source->bytesRead < payload.length().toSizeT());
        REQUIRE(std::ranges::find(source->seekPositions, uint64_t{}) == source->seekPositions.end());
        const auto readCallsBeforeExtraction = source->readCalls.size();
        const auto seeksBeforeExtraction = source->seekPositions.size();
        REQUIRE_EQUAL(reader->item(el::unit::ItemIndex{1U})->extract(payload.length()), payload);
        REQUIRE_EQUAL(source->seekPositions.size(), seeksBeforeExtraction + 3U);
        const auto selectedOffset = source->seekPositions[seeksBeforeExtraction];
        REQUIRE(selectedOffset > payload.length().toRawValue());
        REQUIRE_EQUAL(source->readCalls[readCallsBeforeExtraction].first, selectedOffset);
        for (auto index = readCallsBeforeExtraction; index < source->readCalls.size(); ++index) {
            REQUIRE_GREATER_EQUAL(source->readCalls[index].first, selectedOffset);
            REQUIRE_LESS_EQUAL(source->readCalls[index].second, 65536U);
        }

        const auto readCallsBeforeCopy = source->readCalls.size();
        const auto seeksBeforeCopy = source->seekPositions.size();
        auto copyWriter = el::compression::zip::ArchiveWriter::create(temporary->path() / "copy.zip"_el);
        copyWriter->addItem(*reader->item(el::unit::ItemIndex{1U}));
        copyWriter->finalize();
        REQUIRE_EQUAL(source->seekPositions.size(), seeksBeforeCopy + 5U);
        REQUIRE_EQUAL(source->seekPositions[seeksBeforeCopy], selectedOffset);
        REQUIRE_EQUAL(source->readCalls[readCallsBeforeCopy].first, selectedOffset);
        for (auto index = readCallsBeforeCopy; index < source->readCalls.size(); ++index) {
            REQUIRE_GREATER_EQUAL(source->readCalls[index].first, selectedOffset);
            REQUIRE_LESS_EQUAL(source->readCalls[index].second, 65536U);
        }
        reader->close();
        REQUIRE_EQUAL(source->closeCount, std::size_t{1U});
    }

    void testWriterRejectsUnsafeAndCollidingPaths() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "paths.zip"_el;
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            writer->addData(el::mem::ByteBlock({'x'}), el::path::Path::fromPosix("../escape"_el)));
        REQUIRE_THROWS_AS(
            el::err::ParameterError,
            writer->addData(el::mem::ByteBlock({'x'}), el::path::Path::fromPosix("dir\\file"_el)));
        writer->addData(el::mem::ByteBlock({'x'}), el::path::Path::fromPosix("file"_el));
        REQUIRE_THROWS_AS(
            el::compression::zip::ZipError,
            writer->addData(el::mem::ByteBlock({'y'}), el::path::Path::fromPosix("file/child"_el)));
        writer->abort();
        REQUIRE_THROWS_AS(
            el::err::LogicError, writer->addData(el::mem::ByteBlock({'z'}), el::path::Path::fromPosix("later"_el)));
    }

    void testDirectoryFilteringAndExtraction() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto source = temporary->path() / "source"_el;
        source.operations().createDirectoryOrThrow();
        (source / "public"_el).operations().createDirectoryOrThrow();
        (source / "private"_el).operations().createDirectoryOrThrow();
        (source / "public/a.txt"_el).content().writeDataOrThrow(el::mem::ByteBlock({'a'}));
        (source / "public/a.bin"_el).content().writeDataOrThrow(el::mem::ByteBlock({'b'}));
        (source / "private/b.txt"_el).content().writeDataOrThrow(el::mem::ByteBlock({'c'}));
        const auto archivePath = temporary->path() / "filtered.zip"_el;
        auto callbackCount = std::size_t{};
        auto options = el::compression::zip::ArchiveDirectoryOptions{};
        options.setIncludeGlobs(el::text::StringList{"**/*.txt"_el})
            .setExcludeGlobs(el::text::StringList{"private/**"_el})
            .setFilter([&callbackCount](const el::path::Path &, const el::path::PathInfo &info) {
                ++callbackCount;
                return info.isRegularFile();
            });
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->addDirectory(source, options);
        writer->finalize();
        REQUIRE(callbackCount > 0U);

        auto reader = el::compression::zip::ArchiveReader::create(archivePath);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(reader->item(el::unit::ItemIndex{0U})->path(), el::path::Path::fromPosix("public/a.txt"_el));
        const auto extractionRoot = temporary->path() / "output"_el;
        reader->extractToDirectory(extractionRoot);
        REQUIRE_EQUAL((extractionRoot / "public/a.txt"_el).content().readDataOrThrow(), el::mem::ByteBlock({'a'}));
        REQUIRE_FALSE((extractionRoot / "private/b.txt"_el).info().exists());
    }

    void testExtractionRejectsLinkedParent() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto archivePath = temporary->path() / "linked-parent.zip"_el;
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath);
        writer->addData(el::mem::ByteBlock({'x'}), el::path::Path{"linked/file.txt"_el});
        writer->finalize();

        const auto extractionRoot = temporary->path() / "output"_el;
        const auto outside = temporary->path() / "outside"_el;
        extractionRoot.operations().createDirectoryOrThrow();
        outside.operations().createDirectoryOrThrow();
        auto errorCode = std::error_code{};
        std::filesystem::create_directory_symlink(
            outside.toStdPath(), (extractionRoot / "linked"_el).toStdPath(), errorCode);
        if (errorCode) {
            return;
        }

        auto reader = el::compression::zip::ArchiveReader::create(archivePath);
        REQUIRE_THROWS_AS(el::compression::zip::ZipError, reader->extractToDirectory(extractionRoot));
        REQUIRE_FALSE((outside / "file.txt"_el).info().exists());
    }

    void testFormatTools() {
        namespace tools = el::compression::zip::impl::zipTools;
        REQUIRE(tools::globMatches("reports/**/*.txt"_el, "reports/2026/september/result.txt"_el));
        REQUIRE(tools::globMatches("reports/??.txt"_el, "reports/ab.txt"_el));
        REQUIRE_FALSE(tools::globMatches("reports/*.txt"_el, "reports/2026/result.txt"_el));
        REQUIRE_EQUAL(
            tools::normalizeEntryPath(
                el::text::StringEncoder{el::text::String{"/root/file.txt"_el}}.encode(el::text::StringEncoding::Utf8),
                false),
            el::path::Path::fromPosix("root/file.txt"_el));
        REQUIRE_THROWS_AS(
            el::compression::zip::ZipError,
            tools::normalizeEntryPath(
                el::text::StringEncoder{el::text::String{"../escape"_el}}.encode(el::text::StringEncoding::Utf8),
                false));
        REQUIRE_FALSE(tools::normalizeEntryPath(el::mem::ByteBlock({0xffU, 'x'}), false).isEmpty());

        const auto expectedTime = el::time::DateTime::fromTimeT(1700000000);
        auto extra = el::mem::ByteWriter{};
        tools::appendExtendedTimestamp(extra, expectedTime);
        REQUIRE_EQUAL(tools::modificationTimeFromExtra(extra.toByteBlock(), {}), expectedTime);
    }

    void testAtomicExtractionFailureCleanup() {
        const auto applicationScope = ApplicationTestScope<>{};
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto destination = temporary->path() / "result.bin"_el;
        auto corrupt = el::mem::ByteBlockEditor{makeDescriptorArchive(true)};
        corrupt.setOrThrow(el::unit::ByteIndex{31U}, el::mem::Byte{'z'});
        auto reader = el::compression::zip::ArchiveReader::create(corrupt);
        REQUIRE_THROWS_AS(
            el::compression::zip::ZipError, reader->item(el::unit::ItemIndex{})->extractToFile(destination));
        REQUIRE_FALSE(destination.info().exists());
        // Corrupt output is removed before a normally closable temporary stream leaves extraction.
        REQUIRE(std::filesystem::is_empty(temporary->path().toStdPath()));
    }

    void testSignedAndUnsignedDataDescriptors() {
        for (const auto signedDescriptor : {false, true}) {
            auto reader = el::compression::zip::ArchiveReader::create(makeDescriptorArchive(signedDescriptor));
            REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{1U});
            REQUIRE_EQUAL(
                reader->item(el::unit::ItemIndex{0U})->extract(el::unit::ByteLength{3U}),
                el::mem::ByteBlock({'a', 'b', 'c'}));
        }

        auto malformed = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        REQUIRE(malformed.setInteger<uint32_t>(el::unit::ByteIndex{34U}, 0U));
        auto reader = el::compression::zip::ArchiveReader::create(malformed);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{1U});
        REQUIRE_THROWS_AS(
            el::compression::zip::ZipError, reader->item(el::unit::ItemIndex{})->extract(el::unit::ByteLength{3U}));
    }

    void testLocalVariableFieldsCannotCrossDirectory() {
        for (const auto field : {26U, 28U}) {
            auto invalid = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
            REQUIRE(invalid.setInteger<uint16_t>(el::unit::ByteIndex{field}, 65535U));
            const auto reader = el::compression::zip::ArchiveReader::create(invalid);
            REQUIRE_THROWS_AS(
                el::compression::zip::ZipError, reader->item(el::unit::ItemIndex{})->extract(el::unit::ByteLength{3U}));
        }
    }

    void testMalformedAndUnsupportedRecords() {
        constexpr auto cDirectoryOffset = uint64_t{46U};
        auto truncated = makeDescriptorArchive(false);
        truncated = truncated.slice({}, truncated.length() - el::unit::ByteLength::one());
        requireOpenReason(truncated, el::compression::zip::ZipErrorReason::MalformedArchive);

        auto encrypted = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        REQUIRE(encrypted.setInteger<uint16_t>(
            el::unit::ByteIndex{cDirectoryOffset + 8U},
            static_cast<uint16_t>(
                el::compression::zip::impl::zipTools::cUtf8Flag |
                el::compression::zip::impl::zipTools::cDescriptorFlag | 0x0001U)));
        requireOpenReason(encrypted, el::compression::zip::ZipErrorReason::UnsupportedFeature);

        auto invalidMethodFlags = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        constexpr auto cInvalidFlags = static_cast<uint16_t>(
            el::compression::zip::impl::zipTools::cUtf8Flag | el::compression::zip::impl::zipTools::cDescriptorFlag |
            0x0004U);
        REQUIRE(invalidMethodFlags.setInteger<uint16_t>(el::unit::ByteIndex{6U}, cInvalidFlags));
        REQUIRE(invalidMethodFlags.setInteger<uint16_t>(el::unit::ByteIndex{cDirectoryOffset + 8U}, cInvalidFlags));
        requireOpenReason(invalidMethodFlags, el::compression::zip::ZipErrorReason::UnsupportedFeature);

        auto inconsistentLocalCrc = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        REQUIRE(inconsistentLocalCrc.setInteger<uint32_t>(el::unit::ByteIndex{14U}, 1U));
        auto inconsistentReader = el::compression::zip::ArchiveReader::create(inconsistentLocalCrc);
        REQUIRE_EQUAL(inconsistentReader->itemCount(), el::unit::ItemCount{1U});
        REQUIRE_THROWS_AS(
            el::compression::zip::ZipError,
            inconsistentReader->item(el::unit::ItemIndex{0U})->extract(el::unit::ByteLength{3U}));

        auto multiDisk = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        const auto endOffset = multiDisk.length() - el::unit::ByteLength{22U};
        REQUIRE(multiDisk.setInteger<uint16_t>(el::unit::ByteIndex{endOffset.toRawValue() + 4U}, 1U));
        requireOpenReason(multiDisk, el::compression::zip::ZipErrorReason::UnsupportedFeature);

        auto dotPath = el::mem::ByteBlockEditor{makeDescriptorArchive(false)};
        dotPath.set(el::unit::ByteIndex{30U}, el::mem::Byte{'.'});
        dotPath.set(el::unit::ByteIndex{cDirectoryOffset + 46U}, el::mem::Byte{'.'});
        requireOpenReason(dotPath, el::compression::zip::ZipErrorReason::UnsafePath);
    }
};
