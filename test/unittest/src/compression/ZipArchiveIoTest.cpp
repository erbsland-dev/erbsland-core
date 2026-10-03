// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteInputStream.hpp>
#include <erbsland/ByteOutputStream.hpp>
#include <erbsland/ByteWriter.hpp>
#include <erbsland/compression/zip/impl/ZipTools.hpp>
#include <erbsland/Literals.hpp>
#include <erbsland/unittest/UnitTest.hpp>
#include <erbsland/zip/ArchiveReader.hpp>
#include <erbsland/zip/ArchiveWriter.hpp>
#include <erbsland/zip/CompressionMethod.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <numeric>
#include <utility>
#include <vector>

using namespace el::text::literals;

TESTED_TARGETS(ArchiveReader ArchiveWriter)
class ZipArchiveIoTest final : public el::UnitTest {
    class SparseInputStream final : public el::stream::ByteInputStream {
    private:
        struct Segment final {
            el::unit::ByteIndex offset{};
            el::mem::ByteBlock data;
        };

    public:
        struct ReadCall final {
            uint64_t offset{};
            std::size_t length{};
        };

    public:
        explicit SparseInputStream(const el::unit::ByteCount length) : _length{length} {}

        void addSegment(const el::unit::ByteIndex offset, el::mem::ByteBlock data) {
            _segments.push_back(Segment{offset, std::move(data)});
        }

    public: // implement InputStream
        [[nodiscard]] auto inputSettings() const noexcept -> const el::stream::InputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

    protected: // implement ByteInputStream
        [[nodiscard]] auto readFromSource(el::mem::ByteSpan destination, ReadDeadline)
            -> el::stream::StreamReadResult<el::unit::ByteCount> override {
            if (!_position.isWithin(_length)) {
                return {el::stream::StreamReadStatus::Finished, el::unit::ByteCount{}};
            }
            const auto readOffset = _position;
            const auto count =
                std::min(el::unit::ByteCount::fromSizeT(destination.size()), _length.remainingFrom(_position));
            std::fill_n(destination.data(), count.toSizeT(), el::mem::Byte{});
            const auto readEnd = _position.advanced(count);
            for (const auto &segment : _segments) {
                const auto segmentEnd = segment.offset.advanced(segment.data.length());
                const auto overlapBegin = std::max(_position, segment.offset);
                const auto overlapEnd = std::min(readEnd, segmentEnd);
                if (overlapBegin >= overlapEnd) {
                    continue;
                }
                const auto sourceIndex = segment.offset.absoluteDistanceTo(overlapBegin).toSizeT();
                const auto destinationIndex = _position.absoluteDistanceTo(overlapBegin).toSizeT();
                const auto overlapLength = overlapBegin.absoluteDistanceTo(overlapEnd).toSizeT();
                std::copy_n(
                    segment.data.span().data() + sourceIndex, overlapLength, destination.data() + destinationIndex);
            }
            _position = readEnd;
            bytesRead += count.toRawValue();
            readCalls.push_back(ReadCall{readOffset.toRawValue(), count.toSizeT()});
            return {el::stream::StreamReadStatus::Data, count};
        }
        [[nodiscard]] auto sourceSupportsPositioning() const noexcept -> bool override { return true; }
        [[nodiscard]] auto sourcePosition() const -> el::unit::ByteIndex override { return _position; }
        auto setSourcePosition(const el::unit::ByteIndex position) -> el::stream::StreamPositionStatus override {
            _position = position;
            seekPositions.push_back(_position.toRawValue());
            return el::stream::StreamPositionStatus::Success;
        }
        auto moveSourcePosition(const el::stream::StreamPositionOrigin origin, const el::unit::ByteOffset offset)
            -> el::stream::StreamPositionStatus override {
            auto base = el::unit::ByteIndex{};
            if (origin == el::stream::StreamPositionOrigin::Current) {
                base = _position;
            } else if (origin == el::stream::StreamPositionOrigin::End) {
                base = el::unit::ByteIndex::end(_length);
            }
            _position = base.movedOrThrow(offset);
            return el::stream::StreamPositionStatus::Success;
        }

    public:
        uint64_t bytesRead{};
        std::vector<uint64_t> seekPositions;
        std::vector<ReadCall> readCalls;

    private:
        el::unit::ByteCount _length{};
        el::unit::ByteIndex _position{};
        std::vector<Segment> _segments;
        el::stream::InputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

    class TrackingOutputStream final : public el::stream::ByteOutputStream {
    public: // implement OutputStream
        [[nodiscard]] auto outputSettings() const noexcept -> const el::stream::OutputStreamSettings & override {
            return _settings;
        }
        [[nodiscard]] auto state() const noexcept -> el::stream::StreamState override { return _state; }
        [[nodiscard]] auto isReady() const noexcept -> bool override { return true; }
        [[nodiscard]] auto waitForReady() -> el::stream::StreamWaitStatus override {
            return el::stream::StreamWaitStatus::Ready;
        }
        auto flush() -> el::stream::StreamWriteStatus override { return el::stream::StreamWriteStatus::Success; }
        auto close() -> el::stream::StreamCloseStatus override {
            _state = el::stream::StreamState::Closed;
            return el::stream::StreamCloseStatus::Closed;
        }
        void abort() noexcept override { _state = el::stream::StreamState::Closed; }

    public: // implement ByteOutputStream
        auto write(const el::mem::ConstByteSpan bytes) -> el::stream::StreamWriteStatus override {
            writeLengths.push_back(bytes.size());
            data.append(bytes);
            return el::stream::StreamWriteStatus::Success;
        }

    public:
        el::mem::ByteBlockEditor data;
        std::vector<std::size_t> writeLengths;

    private:
        el::stream::OutputStreamSettings _settings;
        el::stream::StreamState _state{el::stream::StreamState::Open};
    };

public:
    void testZip64RangeOverflow() {
        namespace tools = el::compression::zip::impl::zipTools;
        auto end = el::mem::ByteWriter{};
        end.writeUInt32(tools::cZip64EndSignature)
            .writeUInt64(44U)
            .writeUInt16(45U)
            .writeUInt16(45U)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt64(0U)
            .writeUInt64(0U)
            .writeUInt64(0U)
            .writeUInt64(0U)
            .writeUInt32(tools::cZip64LocatorSignature)
            .writeUInt32(0U)
            .writeUInt64(0U)
            .writeUInt32(1U)
            .writeUInt32(tools::cEndSignature)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0xffffU)
            .writeUInt16(0xffffU)
            .writeUInt32(0xffffffffU)
            .writeUInt32(0xffffffffU)
            .writeUInt16(0U);
        const auto valid = end.toByteBlock();
        REQUIRE(el::compression::zip::ArchiveReader::create(valid)->isZip64());
        for (
            const auto field :
            {el::unit::ByteIndex{4U}, el::unit::ByteIndex{40U}, el::unit::ByteIndex{48U}, el::unit::ByteIndex{64U}}) {
            for (
                const auto value : {std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max() - 16U}) {
                auto invalid = el::mem::ByteBlockEditor{valid};
                REQUIRE(invalid.setInteger<uint64_t>(field, value));
                REQUIRE_THROWS_AS(el::compression::zip::ZipError, el::compression::zip::ArchiveReader::create(invalid));
            }
        }
    }

    void testSparseZip64DirectoryRead() {
        namespace tools = el::compression::zip::impl::zipTools;
        constexpr auto cDirectoryOffset = uint64_t{0x100001000U};
        constexpr auto cDosDate = uint16_t{0x5821U};

        auto directory = el::mem::ByteWriter{};
        directory.writeUInt32(tools::cCentralHeaderSignature)
            .writeUInt16(static_cast<uint16_t>((3U << 8U) | 20U))
            .writeUInt16(20U)
            .writeUInt16(tools::cUtf8Flag)
            .writeUInt16(static_cast<uint16_t>(el::compression::zip::CompressionMethod::Stored))
            .writeUInt16(0U)
            .writeUInt16(cDosDate)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt16(1U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt32(static_cast<uint32_t>(0100644U << 16U))
            .writeUInt32(0U)
            .writeByte(el::mem::Byte{'x'});
        const auto directoryLength = directory.length().toRawValue();
        const auto zip64Offset = cDirectoryOffset + directoryLength;

        auto end = el::mem::ByteWriter{};
        end.writeUInt32(tools::cZip64EndSignature)
            .writeUInt64(44U)
            .writeUInt16(static_cast<uint16_t>((3U << 8U) | 63U))
            .writeUInt16(45U)
            .writeUInt32(0U)
            .writeUInt32(0U)
            .writeUInt64(1U)
            .writeUInt64(1U)
            .writeUInt64(directoryLength)
            .writeUInt64(cDirectoryOffset)
            .writeUInt32(tools::cZip64LocatorSignature)
            .writeUInt32(0U)
            .writeUInt64(zip64Offset)
            .writeUInt32(1U)
            .writeUInt32(tools::cEndSignature)
            .writeUInt16(0U)
            .writeUInt16(0U)
            .writeUInt16(0xffffU)
            .writeUInt16(0xffffU)
            .writeUInt32(0xffffffffU)
            .writeUInt32(0xffffffffU)
            .writeUInt16(0U);
        const auto archiveLength = zip64Offset + end.length().toRawValue();
        auto source = std::make_shared<SparseInputStream>(el::unit::ByteCount::fromInteger(archiveLength));
        source->addSegment(el::unit::ByteIndex{cDirectoryOffset}, directory.toByteBlock());
        source->addSegment(el::unit::ByteIndex::fromInteger(zip64Offset), end.toByteBlock());

        const auto reader = el::compression::zip::ArchiveReader::create(source);
        REQUIRE(reader->isZip64());
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{1U});
        REQUIRE_EQUAL(reader->item(el::unit::ItemIndex{})->path(), el::path::Path::fromPosix("x"_el));
        REQUIRE_EQUAL(source->readCalls.size(), std::size_t{3U});
        REQUIRE_EQUAL(source->readCalls[0U].offset, archiveLength - 65557U);
        REQUIRE_EQUAL(source->readCalls[0U].length, std::size_t{65557U});
        REQUIRE_EQUAL(source->readCalls[1U].offset, zip64Offset);
        REQUIRE_EQUAL(source->readCalls[1U].length, std::size_t{56U});
        REQUIRE_EQUAL(source->readCalls[2U].offset, cDirectoryOffset);
        REQUIRE_EQUAL(source->readCalls[2U].length, static_cast<std::size_t>(directoryLength));
        REQUIRE(std::ranges::find(source->seekPositions, uint64_t{}) == source->seekPositions.end());
    }

    void testCentralDirectoryIsWrittenPerEntry() {
        constexpr auto cEntryCount = std::size_t{32U};
        auto destination = std::make_shared<TrackingOutputStream>();
        auto writer = el::compression::zip::ArchiveWriter::create(destination);
        auto options = el::compression::zip::ArchiveEntryOptions{};
        options.setCompressionMethod(el::compression::zip::CompressionMethod::Stored);
        for (auto index = std::size_t{}; index < cEntryCount; ++index) {
            auto name = el::text::String{"entry"_el};
            name = name.inserted(el::unit::CpIndex::end(name.characterLength()), el::text::String::fromInteger(index));
            writer->addData(el::mem::ByteBlock{}, el::path::Path::fromPosix(name), options);
        }
        const auto writesBeforeFinalization = destination->writeLengths.size();
        writer->finalize();

        const auto finalizationCalls = destination->writeLengths.size() - writesBeforeFinalization;
        REQUIRE_EQUAL(finalizationCalls, cEntryCount + 1U);
        const auto firstFinalizationWrite = destination->writeLengths.begin() +
            static_cast<std::vector<std::size_t>::difference_type>(writesBeforeFinalization);
        const auto serializedLength =
            std::accumulate(firstFinalizationWrite, destination->writeLengths.end(), std::size_t{});
        const auto maximumWrite = *std::max_element(firstFinalizationWrite, destination->writeLengths.end());
        REQUIRE(maximumWrite < serializedLength);

        const auto reader = el::compression::zip::ArchiveReader::create(destination->data);
        REQUIRE_EQUAL(reader->itemCount(), el::unit::ItemCount{cEntryCount});
    }
};
