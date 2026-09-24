// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/zip/ArchiveEntryOptions.hpp>
#include <erbsland/compression/zip/ArchiveItem.hpp>
#include <erbsland/compression/zip/ArchiveReader.hpp>
#include <erbsland/compression/zip/ArchiveWriter.hpp>
#include <erbsland/compression/zip/ArchiveWriterOptions.hpp>
#include <erbsland/compression/zip/CompressionMethod.hpp>
#include <erbsland/compression/zip/Zip64Policy.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/path/PathContent.hpp>
#include <erbsland/path/PathOperations.hpp>
#include <erbsland/path/TempDirectory.hpp>
#include <erbsland/system/Subprocess.hpp>
#include <erbsland/system/SubprocessOptions.hpp>
#include <erbsland/system/SubprocessOutputMode.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ArchiveReader ArchiveItem ArchiveWriter ArchiveEntryOptions ArchiveWriterOptions Zip64Policy)
class ZipRandomizedInteropTest final : public el::UnitTest {
private:
    struct DataCase final {
        el::text::String path;
        el::mem::ByteBlock data;
    };

private:
    static constexpr auto cSeed = uint32_t{0x7a49504dU};
    static constexpr auto cComment = "Erbsland ZIP interoperability – UTF-8"_el;

private:
    [[nodiscard]] static auto nextRandom(uint32_t &state) noexcept -> uint32_t {
        state ^= state << 13U;
        state ^= state >> 17U;
        state ^= state << 5U;
        return state;
    }

    [[nodiscard]] static auto createData(const std::size_t length, uint32_t state) -> el::mem::ByteBlock {
        auto bytes = std::vector<uint8_t>{};
        bytes.reserve(length);
        for (auto index = std::size_t{}; index < length; ++index) {
            const auto random = nextRandom(state);
            if (index % 4096U < 384U) {
                bytes.push_back(static_cast<uint8_t>((index + state) % 19U));
            } else if (index % 8192U < 6144U) {
                bytes.push_back(static_cast<uint8_t>(random & 0xffU));
            } else {
                bytes.push_back(static_cast<uint8_t>((index / 13U) & 0x0fU));
            }
        }
        return el::mem::ByteBlock::fromVector(bytes);
    }

    [[nodiscard]] static auto casePath(const std::size_t index) -> el::text::String {
        if (index % 7U == 0U) {
            return el::text::StringFormat{"cases/ümlaut-{:03}.bin"_el}.build(index);
        }
        return el::text::StringFormat{"cases/case-{:03}.bin"_el}.build(index);
    }

    [[nodiscard]] static auto dataCases() -> std::vector<DataCase> {
        auto sizes = std::vector<std::size_t>{};
        for (auto size = std::size_t{}; size <= 33U; ++size) {
            sizes.push_back(size);
        }
        sizes.insert(sizes.end(), {255U, 256U, 257U, 4095U, 4096U, 4097U, 65535U, 65536U, 65537U});
        auto state = cSeed;
        for (auto index = std::size_t{}; index < 12U; ++index) {
            sizes.push_back(nextRandom(state) % (192U * 1024U + 1U));
        }
        auto result = std::vector<DataCase>{};
        result.reserve(sizes.size());
        for (auto index = std::size_t{}; index < sizes.size(); ++index) {
            result.push_back({casePath(index), createData(sizes[index], cSeed ^ static_cast<uint32_t>(index))});
        }
        return result;
    }

    [[nodiscard]] static auto coreMethod(const std::size_t index) -> el::compression::zip::CompressionMethod {
        constexpr auto methods = std::array{
            el::compression::zip::CompressionMethod::Stored,
            el::compression::zip::CompressionMethod::Deflate,
            el::compression::zip::CompressionMethod::Bzip2,
            el::compression::zip::CompressionMethod::Lzma,
            el::compression::zip::CompressionMethod::Zstandard};
        return methods[index % methods.size()];
    }

    [[nodiscard]] static auto rustMethod(const std::size_t index) -> el::compression::zip::CompressionMethod {
        constexpr auto methods = std::array{
            el::compression::zip::CompressionMethod::Stored,
            el::compression::zip::CompressionMethod::Deflate,
            el::compression::zip::CompressionMethod::Bzip2,
            el::compression::zip::CompressionMethod::Zstandard};
        return methods[index % methods.size()];
    }

    [[nodiscard]] static auto counterpartPath() -> el::path::Path {
        auto result = el::path::Path{el::unittest::fh::unitTestExecutablePath()}.parent();
#if defined(_WIN32)
        result /= "erbsland-core-zip-interop-counterpart.exe"_el;
#else
        result /= "erbsland-core-zip-interop-counterpart"_el;
#endif
        return result;
    }

    [[nodiscard]] static auto nativePathText(const el::path::Path &path) -> el::text::String {
        return el::text::StringConverter{path.toStdPath().string()}.toString();
    }

    static void writeSources(const el::path::Path &workspace, const std::vector<DataCase> &cases) {
        for (auto index = std::size_t{}; index < cases.size(); ++index) {
            (workspace / el::text::StringFormat{"case-{:03}.bin"_el}.build(index))
                .content()
                .writeDataOrThrow(cases[index].data);
        }
    }

    static void writeCoreArchive(
        const el::path::Path &archivePath, const std::vector<DataCase> &cases, const bool zip64) {
        auto writerOptions = el::compression::zip::ArchiveWriterOptions{};
        if (zip64) {
            writerOptions.setZip64Policy(el::compression::zip::Zip64Policy::Always);
        }
        auto writer = el::compression::zip::ArchiveWriter::create(archivePath, writerOptions);
        writer->setComment(cComment);
        const auto modificationTime = el::time::DateTime::fromIsoStringOrThrow("2024-02-03T04:05:06"_el);
        for (auto index = std::size_t{}; index < cases.size(); ++index) {
            auto options = el::compression::zip::ArchiveEntryOptions{};
            options.setCompressionMethod(coreMethod(index)).setModificationTime(modificationTime);
            writer->addData(cases[index].data, el::path::Path::fromPosix(cases[index].path), options);
        }
        writer->finalize();
    }

    void runCounterpart(const el::path::Path &workspace, const std::size_t caseCount) {
        auto options = el::system::SubprocessOptions{};
        options.setInheritStandardInput(false)
            .setStandardOutputMode(el::system::SubprocessOutputMode::Discard)
            .setStandardErrorMode(el::system::SubprocessOutputMode::Capture);
        auto process = el::system::Subprocess::start(
            counterpartPath(),
            el::text::StringList{
                "verify-batch"_el, nativePathText(workspace), el::text::String::fromInteger(caseCount)},
            options);
        const auto status = process.wait();
        const auto error = el::text::StringConverter{process.standardError()}.toString();
        if (!error.isEmpty()) {
            _context = el::text::StringFormat{"{}\nRust counterpart error:\n{}"_el}.build(_context, error);
        }
        REQUIRE(status.isSuccess());
    }

    void verifyRustArchive(
        const el::path::Path &archivePath, const std::vector<DataCase> &cases, const el::text::String &variant) {
        _context = el::text::StringFormat{"Rust ZIP variant: {}"_el}.build(variant);
        const auto reader = el::compression::zip::ArchiveReader::create(archivePath);
        REQUIRE_EQUAL(reader->comment(), cComment);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{cases.size()});
        for (auto index = std::size_t{}; index < cases.size(); ++index) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto item = reader->item(el::unit::ItemIndex::fromSizeT(index));
                    REQUIRE(item != nullptr);
                    REQUIRE_EQUAL(item->path(), el::path::Path::fromPosix(cases[index].path));
                    REQUIRE_EQUAL(item->compressionMethod(), rustMethod(index));
                    REQUIRE_EQUAL(item->uncompressedLength(), cases[index].data.length());
                    REQUIRE_EQUAL(item->extract(el::unit::ByteLength{256U * 1024U}), cases[index].data);
                },
                [&]() -> std::string {
                    return el::text::StringConverter{
                        el::text::StringFormat{"variant: {}\nentry: {}\npath: {}"_el}.build(
                            variant, index, cases[index].path)}
                        .toStdString();
                });
        }
    }

public: // implement UnitTest
    auto additionalErrorMessages() -> std::string override {
        return el::text::StringConverter{_context}.toStdString();
    }

public:
    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testBidirectionalRust() {
        const auto cases = dataCases();
        const auto temporary = el::path::Path::systemTempDirectoryOrThrow().operations().createTempDirectoryOrThrow();
        const auto workspace = temporary->path();
        writeSources(workspace, cases);
        _context = "Core classic ZIP -> Rust"_el;
        writeCoreArchive(workspace / "core-classic.zip"_el, cases, false);
        _context = "Core ZIP64 -> Rust"_el;
        writeCoreArchive(workspace / "core-zip64.zip"_el, cases, true);
        runCounterpart(workspace, cases.size());
        verifyRustArchive(workspace / "rust-classic.zip"_el, cases, "classic"_el);
        verifyRustArchive(workspace / "rust-stream.zip"_el, cases, "stream-data-descriptors"_el);
        verifyRustArchive(workspace / "rust-zip64.zip"_el, cases, "forced-entry-zip64"_el);
        _context = {};
    }

private:
    el::text::String _context;
};
