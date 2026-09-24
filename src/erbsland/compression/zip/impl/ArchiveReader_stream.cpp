// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveReader.hpp"

#include "ZipTools.hpp"

#include "../../../compression/CompressionError.hpp"
#include "../../../compression/CompressionFormat.hpp"
#include "../../../compression/DecompressionOptions.hpp"
#include "../../../err/LogicError.hpp"
#include "../../../err/ParameterError.hpp"
#include "../../../stream/StreamPositionStatus.hpp"
#include "../../../stream/StreamReadStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../unit/ByteIndex.hpp"
#include "../../impl/CompressionCodec.hpp"
#include "../../impl/CompressionCrc32.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

void ArchiveReader::extractTo(
    const ZipEntryRecord &record,
    unit::ByteLength maximum,
    const compression::impl::CodecOutput::Write &output,
    ZipProgressFn progress) const {
    if (!maximum.isFinite()) {
        throw err::ParameterError{"Extraction maximum must be finite."_el, "maximum"_el};
    }
    maximum = std::min(maximum, _options.maximumItemLength());
    if (record.uncompressedLength > maximum) {
        throwArchiveError(
            ZipErrorReason::ResourceLimit,
            ZipOperationPhase::Decompression,
            "ZIP output limit exceeded"_el,
            "Declared output exceeds the configured limit."_el);
    }
    const auto lock = std::lock_guard{_sourceMutex};
    if (!_isOpen || !_source) {
        throw err::LogicError{"ZIP reader is closed."_el};
    }
    const auto entry = readEntryData(record, false);
    if (_source->setPosition(entry.payloadOffset) != stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::Payload,
            "ZIP seek timed out"_el,
            "Payload positioning failed."_el);
    }
    if (!progress) {
        progress = _options.progress();
    }
    std::exception_ptr callbackError;
    const auto observer = [&](const CompressionProgress &value) {
        if (!progress || value.phase == CompressionPhase::Completed) {
            return CompressionProgressAction::Continue;
        }
        try {
            return progress(ZipProgress{value, record.path, ZipOperationPhase::Decompression});
        } catch (...) {
            callbackError = std::current_exception();
            throw;
        }
    };
    unit::ByteLength remaining = record.compressedLength;
    unit::ByteLength produced;
    compression::impl::CompressionCrc32 crc;
    auto read = [&]() -> mem::ByteBlock {
        if (remaining.toRawValue() == 0U) {
            return {};
        }
        const auto value = _source->read(std::min(remaining, unit::ByteLength{65536U}));
        if (value != stream::StreamReadStatus::Data) {
            throwArchiveError(
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Payload,
                "ZIP payload read failed"_el,
                "The payload ended early or timed out."_el);
        }
        remaining -= value.data().length();
        return value.data();
    };
    const auto sink = [&](mem::ConstByteSpan bytes) {
        const auto next = produced.addedOrThrow(unit::ByteLength::fromSizeT(bytes.size()));
        if (next > record.uncompressedLength || next > maximum) {
            throwArchiveError(
                ZipErrorReason::ResourceLimit,
                ZipOperationPhase::Decompression,
                "ZIP output limit exceeded"_el,
                "Actual output exceeds its declared or configured limit."_el);
        }
        output(bytes);
        crc.update(bytes);
        produced = next;
    };
    if (record.compressionMethod == CompressionMethod::Stored) {
        CompressionProgress state;
        state.inputTotal = record.compressedLength;
        state.outputTotal = record.uncompressedLength;
        auto notify = [&]() {
            if (observer(state) == CompressionProgressAction::Cancel && state.phase != CompressionPhase::Completed) {
                throw compression::CompressionError{CompressionErrorReason::Cancelled, "ZIP extraction cancelled."_el};
            }
        };
        notify();
        while (remaining.toRawValue()) {
            const auto bytes = read();
            sink(bytes.span());
            state.transferred = {produced, produced};
            notify();
        }
        state.phase = CompressionPhase::Completed;
        notify();
    } else {
        auto options = DecompressionOptions{}
                           .setExpectedOutputLength(record.uncompressedLength)
                           .setMaximumOutputLength(maximum)
                           .setMaximumWorkspaceLength(_options.maximumCodecWorkspace());
        auto transfer = CompressionTransferOptions{}
                            .setInputLength(record.compressedLength)
                            .setMaximumBufferedLength(_options.maximumBufferedItemLength())
                            .setProgress(observer);
        const auto codec = compression::impl::CompressionCodec::create(
            zipTools::algorithmForMethod(record.compressionMethod), CompressionFormat::Zip);
        try {

            codec->decompress(read, sink, transfer, options);
        } catch (const compression::CompressionError &) {
            if (callbackError) {
                std::rethrow_exception(callbackError);
            }
            throw ZipError{
                ZipErrorContext{
                    ZipErrorReason::CodecFailure, ZipOperationPhase::Decompression, "ZIP decompression failed"_el}
                    .setItemPath(record.path),
                std::current_exception()};
        }
    }
    if (produced != record.uncompressedLength || crc.value() != record.crc32) {
        throwArchiveError(
            ZipErrorReason::IntegrityFailure,
            ZipOperationPhase::Decompression,
            "ZIP integrity check failed"_el,
            "Length or CRC differs from the central directory."_el);
    }
    if (progress) {
        progress(
            ZipProgress{
                CompressionProgress{
                    {record.compressedLength, produced},
                    record.compressedLength,
                    record.uncompressedLength,
                    CompressionPhase::Completed},
                record.path,
                ZipOperationPhase::Decompression});
    }
}

void ArchiveReader::copyPayload(
    const ZipEntryRecord &record, const compression::impl::CodecOutput::Write &output) const {
    const auto lock = std::lock_guard{_sourceMutex};
    if (!_isOpen || !_source) {
        throw err::LogicError{"ZIP reader is closed."_el};
    }
    const auto entry = readEntryData(record, false);
    if (_source->setPosition(entry.payloadOffset) != stream::StreamPositionStatus::Success) {
        throwArchiveError(
            ZipErrorReason::StreamFailure,
            ZipOperationPhase::Payload,
            "ZIP seek timed out"_el,
            "Payload positioning failed."_el);
    }
    auto remaining = record.compressedLength;
    while (remaining.toRawValue()) {
        const auto block = _source->read(std::min(remaining, unit::ByteLength{65536U}));
        if (block != stream::StreamReadStatus::Data) {
            throwArchiveError(
                ZipErrorReason::StreamFailure,
                ZipOperationPhase::Payload,
                "ZIP copy read failed"_el,
                "The payload ended early or timed out."_el);
        }
        output(block.data().span());
        remaining -= block.data().length();
    }
}

}
