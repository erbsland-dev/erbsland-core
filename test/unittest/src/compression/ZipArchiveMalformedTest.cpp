// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteBlockInputStream.hpp>
#include <erbsland/ByteWriter.hpp>
#include <erbsland/compression/zip/impl/ZipTools.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/zip/ArchiveReader.hpp>
#include <erbsland/zip/ZipError.hpp>

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <string>

using namespace el::text::literals;

TESTED_TARGETS(ArchiveReader)
class ZipArchiveMalformedTest final : public el::UnitTest {
private:
    // Bits select uncompressed size, compressed size, local-header offset, and starting disk, respectively.
    [[nodiscard]] static auto extraLength(const unsigned fields) -> uint16_t {
        auto result = uint16_t{};
        for (const auto field : {1U, 2U, 4U}) {
            if ((fields & field) != 0U) {
                result += 8U;
            }
        }
        if ((fields & 8U) != 0U) {
            result += 4U;
        }
        return result;
    }

    [[nodiscard]] static auto makeExtra(const unsigned fields, const uint16_t length) -> el::mem::ByteBlock {
        if (fields == 0U) {
            return {};
        }
        auto writer = el::mem::ByteWriter{};
        writer.writeUInt16(el::compression::zip::impl::zipTools::cZip64ExtraId).writeUInt16(length);
        writer.writeBytes(el::mem::ByteBlock{el::unit::ByteCount::fromInteger(length), el::mem::Byte{}});
        return writer.toByteBlock();
    }

    [[nodiscard]] static auto makeArchive(
        const unsigned centralFields,
        const uint16_t centralLength,
        const unsigned localFields = 0U,
        const uint16_t localLength = 0U) -> el::mem::ByteBlock {
        namespace tools = el::compression::zip::impl::zipTools;
        const auto localExtra = makeExtra(localFields, localLength);
        const auto centralExtra = makeExtra(centralFields, centralLength);
        auto writer = el::mem::ByteWriter{};
        // One empty Stored file, with ZIP64 values (all zero) conditionally replacing classic sentinel fields.
        writer.writeUInt32(tools::cLocalHeaderSignature)
            .writeUInt16(45U)
            .writeUInt16(tools::cUtf8Flag)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0x0021U)
            .writeUInt32(0U)
            .writeUInt32((localFields & 2U) != 0U ? 0xffffffffU : 0U)
            .writeUInt32((localFields & 1U) != 0U ? 0xffffffffU : 0U)
            .writeUInt16(1U)
            .writeUInt16(static_cast<uint16_t>(localExtra.length().toRawValue()))
            .writeByte(el::mem::Byte{'x'})
            .writeBytes(localExtra);
        const auto directoryOffset = writer.length().toRawValue();
        writer.writeUInt32(tools::cCentralHeaderSignature)
            .writeUInt16(45U)
            .writeUInt16(45U)
            .writeUInt16(tools::cUtf8Flag)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0x0021U)
            .writeUInt32(0U)
            .writeUInt32((centralFields & 2U) != 0U ? 0xffffffffU : 0U)
            .writeUInt32((centralFields & 1U) != 0U ? 0xffffffffU : 0U)
            .writeUInt16(1U)
            .writeUInt16(static_cast<uint16_t>(centralExtra.length().toRawValue()))
            .writeUInt16(0U)
            .writeUInt16((centralFields & 8U) != 0U ? 0xffffU : 0U)
            .writeUInt16(0U)
            .writeUInt32(0U)
            .writeUInt32((centralFields & 4U) != 0U ? 0xffffffffU : 0U)
            .writeByte(el::mem::Byte{'x'})
            .writeBytes(centralExtra);
        const auto directoryLength = writer.length().toRawValue() - directoryOffset;
        writer.writeUInt32(tools::cEndSignature)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(1U)
            .writeUInt16(1U)
            .writeUInt32(static_cast<uint32_t>(directoryLength))
            .writeUInt32(static_cast<uint32_t>(directoryOffset))
            .writeUInt16(0U);
        return writer.toByteBlock();
    }

    void requireMalformed(const el::mem::ByteBlock &archive) {
        const auto source = std::make_shared<el::stream::ByteBlockInputStream>(archive);
        auto rejected = false;
        try {
            static_cast<void>(el::compression::zip::ArchiveReader::create(source));
        } catch (const el::compression::zip::ZipError &error) {
            rejected = true;
            REQUIRE_EQUAL(error.reasonCode(), el::compression::zip::ZipErrorReason::MalformedArchive);
        }
        REQUIRE(rejected);
        REQUIRE_EQUAL(source->state(), el::stream::StreamState::Closed);
    }

    void checkMutations(const bool exhaustive) {
        for (const auto fields : {0U, 3U, 15U}) {
            const auto original = makeArchive(fields, extraLength(fields));
            const auto offsetStep = exhaustive ? 1U : 8U;
            for (auto offset = std::size_t{}; offset < original.length().toSizeT(); offset += offsetStep) {
                const auto step = exhaustive ? 1U : 255U;
                for (auto value = 0U; value <= 255U; value += step) {
                    auto mutated = el::mem::ByteBlockEditor{original};
                    mutated.set(el::unit::ByteIndex::fromSizeT(offset), el::mem::Byte{static_cast<uint8_t>(value)});
                    runWithContext(
                        SOURCE_LOCATION(),
                        [&]() -> void {
                            try {
                                static_cast<void>(el::compression::zip::ArchiveReader::create(mutated));
                            } catch (const el::compression::zip::ZipError &) {
                                // Mutations may remain valid; rejection must be a ZIP runtime diagnostic.
                            }
                        },
                        [&]() -> std::string {
                            return std::format("fields={}, byte offset={}, value={}", fields, offset, value);
                        });
                }
            }
        }
    }

public:
    void testZip64CentralExtraLengthsLight() {
        const auto required = extraLength(15U);
        const auto valid = el::compression::zip::ArchiveReader::create(makeArchive(15U, required));
        REQUIRE(valid->isZip64());
        WITH_CONTEXT(requireMalformed(makeArchive(15U, 0U)));
        WITH_CONTEXT(requireMalformed(makeArchive(15U, required - 1U)));
        WITH_CONTEXT(requireMalformed(makeArchive(15U, required + 1U)));
    }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testZip64CentralExtraLengths() {
        for (auto fields = 1U; fields < 16U; ++fields) {
            const auto required = extraLength(fields);
            const auto valid = el::compression::zip::ArchiveReader::create(makeArchive(fields, required));
            REQUIRE(valid->isZip64());
            REQUIRE_EQUAL(valid->itemCount(), el::unit::ItemCount::one());
            for (auto length = uint16_t{}; length < required; ++length) {
                WITH_CONTEXT(requireMalformed(makeArchive(fields, length)));
            }
            WITH_CONTEXT(requireMalformed(makeArchive(fields, required + 1U)));
        }
    }

    void testZip64LocalExtraLengths() {
        for (auto fields = 1U; fields < 4U; ++fields) {
            const auto required = extraLength(fields);
            const auto valid =
                el::compression::zip::ArchiveReader::create(makeArchive(fields, required, fields, required));
            REQUIRE(valid->item(el::unit::ItemIndex{})->extract(el::unit::ByteCount::one()).isEmpty());
            for (auto length = uint16_t{}; length <= required + 1U; ++length) {
                if (length == required) {
                    continue;
                }
                const auto reader =
                    el::compression::zip::ArchiveReader::create(makeArchive(fields, required, fields, length));
                REQUIRE_THROWS_AS(
                    el::compression::zip::ZipError,
                    reader->item(el::unit::ItemIndex{})->extract(el::unit::ByteCount::one()));
            }
        }
    }

    void testTruncatedArchives() {
        const auto original = makeArchive(15U, extraLength(15U));
        for (auto length = el::unit::ByteCount{}; length < original.length(); ++length) {
            WITH_CONTEXT(requireMalformed(original.slice(el::unit::ByteIndex{}, length)));
        }
    }

    void testMutationsLight() { WITH_CONTEXT(checkMutations(false)); }

    SKIP_BY_DEFAULT()
    TAGS(FullRun)
    void testMutations() { WITH_CONTEXT(checkMutations(true)); }
};
