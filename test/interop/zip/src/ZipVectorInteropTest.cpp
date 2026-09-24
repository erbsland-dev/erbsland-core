// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/compression/zip/ArchiveItem.hpp>
#include <erbsland/compression/zip/ArchiveReader.hpp>
#include <erbsland/compression/zip/CompressionMethod.hpp>
#include <erbsland/err/RuntimeError.hpp>
#include <erbsland/mem/ByteBlock.hpp>
#include <erbsland/path/Path.hpp>
#include <erbsland/text/CharSet.hpp>
#include <erbsland/text/Literals.hpp>
#include <erbsland/text/StringEncoder.hpp>
#include <erbsland/text/StringEncoding.hpp>
#include <erbsland/text/StringConverter.hpp>
#include <erbsland/text/StringFormat.hpp>
#include <erbsland/text/StringList.hpp>
#include <erbsland/time/DateTime.hpp>
#include <erbsland/unittest/UnitTest.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ArchiveReader ArchiveItem CompressionMethod)
class ZipVectorInteropTest final : public el::UnitTest {
private:
    struct EntryCase final {
        el::path::Path path;
        bool directory{};
        el::compression::zip::CompressionMethod method{};
        el::text::String comment;
        el::time::DateTime timestamp;
        el::text::String payloadCase;
    };

    struct ArchiveCase final {
        el::text::String name;
        el::text::String producer;
        el::text::String version;
        el::text::String variant;
        el::text::String comment;
        bool zip64{};
        std::vector<EntryCase> entries;
    };

private:
    [[nodiscard]] static auto splitFields(const el::text::String &line, const std::size_t expectedCount)
        -> el::text::StringList {
        const auto fields = el::text::StringList::fromSplit(
            line, el::text::CharSet{U'\t'}, el::unit::ItemCount::infinite(), true);
        if (fields.count() != el::unit::ItemCount{expectedCount}) {
            throw el::err::RuntimeError{
                el::text::StringFormat{"ZIP vector row has {} fields instead of {}."_el}.build(
                    fields.count(), expectedCount)};
        }
        return fields;
    }

    [[nodiscard]] static auto field(const el::text::StringList &fields, const std::size_t index) -> el::text::String {
        return fields.get(el::unit::ItemIndex::fromSizeT(index));
    }

    [[nodiscard]] static auto readRows(const std::string &relativePath) -> el::text::StringList {
        const auto content = el::text::StringConverter{el::unittest::fh::readDataText(relativePath)}.toString();
        return el::text::StringList::fromSplit(
            content, el::text::CharSet{U'\n'}, el::unit::ItemCount::infinite(), true);
    }

    [[nodiscard]] static auto parseBoolean(const el::text::String &value) -> bool {
        if (value == "yes"_el) {
            return true;
        }
        if (value == "no"_el) {
            return false;
        }
        throw el::err::RuntimeError{"ZIP vector boolean must be 'yes' or 'no'."_el};
    }

    [[nodiscard]] static auto parseMethod(const el::text::String &value)
        -> el::compression::zip::CompressionMethod {
        using Method = el::compression::zip::CompressionMethod;
        if (value == "stored"_el) {
            return Method::Stored;
        }
        if (value == "deflate"_el) {
            return Method::Deflate;
        }
        if (value == "bzip2"_el) {
            return Method::Bzip2;
        }
        if (value == "lzma"_el) {
            return Method::Lzma;
        }
        if (value == "zstandard"_el) {
            return Method::Zstandard;
        }
        throw el::err::RuntimeError{"ZIP vector compression method is unknown."_el};
    }

    [[nodiscard]] static auto optionalText(const el::text::String &value) -> el::text::String {
        return value == "-"_el ? el::text::String{} : value;
    }

    [[nodiscard]] static auto archives() -> std::vector<ArchiveCase> {
        auto result = std::vector<ArchiveCase>{};
        for (const auto &sourceLine : readRows("vectors/manifest.tsv")) {
            const auto line = sourceLine.trimmed();
            if (line.isEmpty() || line.startsWith("#"_el)) {
                continue;
            }
            const auto fields = splitFields(line, 6U);
            result.push_back(
                {.name = field(fields, 0U),
                    .producer = field(fields, 1U),
                    .version = field(fields, 2U),
                    .variant = field(fields, 3U),
                    .comment = optionalText(field(fields, 4U)),
                    .zip64 = parseBoolean(field(fields, 5U))});
        }
        for (const auto &sourceLine : readRows("vectors/entries.tsv")) {
            const auto line = sourceLine.trimmed();
            if (line.isEmpty() || line.startsWith("#"_el)) {
                continue;
            }
            const auto fields = splitFields(line, 7U);
            const auto archiveName = field(fields, 0U);
            const auto archive = std::ranges::find(result, archiveName, &ArchiveCase::name);
            if (archive == result.end()) {
                throw el::err::RuntimeError{"ZIP entry vector references an unknown archive."_el};
            }
            archive->entries.push_back(
                {.path = el::path::Path::fromPosix(field(fields, 1U)),
                    .directory = parseBoolean(field(fields, 2U)),
                    .method = parseMethod(field(fields, 3U)),
                    .comment = optionalText(field(fields, 4U)),
                    .timestamp = el::time::DateTime::fromIsoStringOrThrow(field(fields, 5U)),
                    .payloadCase = field(fields, 6U)});
        }
        return result;
    }

    [[nodiscard]] static auto encode(const el::text::String &value) -> el::mem::ByteBlock {
        return el::text::StringEncoder{value}.encode(el::text::StringEncoding::Utf8);
    }

    [[nodiscard]] static auto payload(const el::text::String &name) -> el::mem::ByteBlock {
        if (name == "directory"_el || name == "empty"_el) {
            return {};
        }
        if (name == "ascii"_el) {
            return encode("The quick brown fox jumps over the lazy dog.\n"_el);
        }
        if (name == "all-bytes"_el) {
            auto bytes = std::vector<uint8_t>{};
            bytes.reserve(256U);
            for (auto value = uint16_t{}; value <= 0xffU; ++value) {
                bytes.push_back(static_cast<uint8_t>(value));
            }
            return el::mem::ByteBlock::fromVector(bytes);
        }
        if (name == "repetitive"_el) {
            auto bytes = std::vector<uint8_t>{};
            bytes.reserve(6208U);
            for (auto index = std::size_t{}; index < 1024U; ++index) {
                bytes.insert(bytes.end(), {'a', 'b', 'c', '1', '2', '3'});
            }
            for (auto value = uint8_t{}; value < 64U; ++value) {
                bytes.push_back(value);
            }
            return el::mem::ByteBlock::fromVector(bytes);
        }
        if (name == "utf8"_el) {
            return encode("Grüezi – こんにちは\n"_el);
        }
        throw el::err::RuntimeError{"ZIP vector payload case is unknown."_el};
    }

    [[nodiscard]] static auto crc32(const el::mem::ByteBlock &data) noexcept -> uint32_t {
        auto crc = uint32_t{0xffffffffU};
        for (const auto byte : data.span()) {
            crc ^= byte.toUInt8();
            for (auto bit = 0U; bit < 8U; ++bit) {
                const auto mask = static_cast<uint32_t>(0U - (crc & 1U));
                crc = (crc >> 1U) ^ (0xedb88320U & mask);
            }
        }
        return ~crc;
    }

    [[nodiscard]] static auto methodIndex(const el::compression::zip::CompressionMethod method) -> std::size_t {
        using Method = el::compression::zip::CompressionMethod;
        switch (method) {
        case Method::Stored:
            return 0U;
        case Method::Deflate:
            return 1U;
        case Method::Bzip2:
            return 2U;
        case Method::Lzma:
            return 3U;
        case Method::Zstandard:
            return 4U;
        default:
            throw el::err::RuntimeError{"ZIP vector compression method is unsupported."_el};
        }
    }

    void verifyArchive(const ArchiveCase &archiveCase) {
        _context = el::text::StringFormat{
            "archive: {}\nproducer: {} {}\nvariant: {}"_el}
                       .build(archiveCase.name, archiveCase.producer, archiveCase.version, archiveCase.variant);
        const auto path = el::path::Path{el::unittest::fh::resolveDataPath("vectors")} / archiveCase.name;
        const auto reader = el::compression::zip::ArchiveReader::create(path);
        REQUIRE_EQUAL(reader->comment(), archiveCase.comment);
        REQUIRE_EQUAL(reader->isZip64(), archiveCase.zip64);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{archiveCase.entries.size()});
        for (auto index = std::size_t{}; index < archiveCase.entries.size(); ++index) {
            runWithContext(
                SOURCE_LOCATION(),
                [&]() -> void {
                    const auto &entry = archiveCase.entries[index];
                    const auto expectedPayload = payload(entry.payloadCase);
                    const auto item = reader->item(el::unit::ItemIndex::fromSizeT(index));
                    REQUIRE(item != nullptr);
                    REQUIRE_EQUAL(item->path(), entry.path);
                    REQUIRE_EQUAL(item->isDirectory(), entry.directory);
                    REQUIRE_EQUAL(item->compressionMethod(), entry.method);
                    REQUIRE_EQUAL(item->comment(), entry.comment);
                    REQUIRE_EQUAL(item->lastModificationTime(), entry.timestamp);
                    REQUIRE_EQUAL(item->uncompressedLength(), expectedPayload.length());
                    REQUIRE_EQUAL(item->crc32(), crc32(expectedPayload));
                    if (!entry.directory) {
                        REQUIRE_EQUAL(item->extract(el::unit::ByteLength{16U * 1024U}), expectedPayload);
                    }
                },
                [&]() -> std::string {
                    return el::text::StringConverter{
                        el::text::StringFormat{"archive: {}\nentry index: {}\nentry path: {}"_el}.build(
                            archiveCase.name, index, archiveCase.entries[index].path.toString())}
                        .toStdString();
                });
        }
    }

public: // implement UnitTest
    auto additionalErrorMessages() -> std::string override {
        return _context.isEmpty() ? std::string{} : el::text::StringConverter{_context}.toStdString();
    }

public:
    void testCommittedVectors() {
        const auto vectorArchives = archives();
        REQUIRE_EQUAL(vectorArchives.size(), std::size_t{10U});
        auto methods = std::array<bool, 5U>{};
        for (const auto &archive : vectorArchives) {
            for (const auto &entry : archive.entries) {
                methods[methodIndex(entry.method)] = true;
            }
            WITH_CONTEXT(verifyArchive(archive));
        }
        REQUIRE(std::ranges::all_of(methods, [](const bool value) { return value; }));
        _context = {};
    }

private:
    el::text::String _context;
};
