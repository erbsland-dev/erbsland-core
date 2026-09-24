// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ArchiveWriter.hpp"

#include "ZipTools.hpp"

#include "../../../err/ParameterError.hpp"
#include "../../../path/PathContent.hpp"
#include "../../../path/PathInfo.hpp"
#include "../../../path/PathReadDataOptions.hpp"
#include "../../../path/SymlinkMode.hpp"
#include "../../../stream/ByteBlockInputStream.hpp"
#include "../../../stream/ByteInputStream.hpp"
#include "../../../stream/StreamReadStatus.hpp"
#include "../../../text/Literals.hpp"
#include "../../../time/DateTime.hpp"
#include "../../ByteCompressor.hpp"
#include "../../impl/CompressionCodec.hpp"
#include "../../impl/CompressionCrc32.hpp"

#include <exception>
#include <limits>
#include <utility>

namespace erbsland::compression::zip::impl {

using namespace text::literals;

void ArchiveWriter::addFile(const path::Path &source, const path::Path &storagePath, ArchiveEntryOptions options) {
    requireWritable();
    if (source.isEmpty()) {
        throw err::ParameterError{"The source file path must not be empty."_el, "source"_el};
    }
    const auto info = source.info();
    if (!info.isRegularFile()) {
        throw err::ParameterError{"Only regular files can be added to a ZIP archive."_el, "source"_el};
    }
    auto resolvedStoragePath = storagePath;
    if (resolvedStoragePath.isEmpty()) {
        resolvedStoragePath =
            _baseDirectory.isEmpty() ? path::Path::fromPosix(source.name()) : source.toRelativeOrThrow(_baseDirectory);
    }
    if (!options.modificationTime().has_value()) {
        options.setModificationTime(info.lastModified());
    }
    auto stream = source.content().openByteInputStream(
        path::PathReadDataOptions{}
            .setStreamSettings(_options.inputStreamSettings())
            .setSymlinkMode(path::SymlinkMode::Follow));
    addStream(std::move(stream), resolvedStoragePath, std::move(options), info.fileSize());
}

void ArchiveWriter::addData(const mem::ByteBlock &data, const path::Path &storagePath, ArchiveEntryOptions options) {
    addStream(std::make_shared<stream::ByteBlockInputStream>(data), storagePath, std::move(options), data.length());
}

void ArchiveWriter::addStream(
    stream::ByteInputStreamPtr source,
    const path::Path &storagePath,
    ArchiveEntryOptions options,
    const unit::ByteLength length) {
    requireWritable();
    if (source == nullptr) {
        throw err::ParameterError{"The ZIP item input stream must not be null."_el, "source"_el};
    }
    const auto method = options.compressionMethod().value_or(_compressionMethod);
    const auto level = options.compressionLevel().value_or(_compressionLevel);
    std::optional<unit::ByteLength> bound;
    if (length.isFinite()) {
        bound = method == CompressionMethod::Stored
            ? length
            : compression::ByteCompressor{zipTools::algorithmForMethod(method), CompressionFormat::Zip, level}
                  .maximumCompressedLength(length);
    }
    const auto entryOffset = _offset;
    try {
        addCompressed(
            {},
            storagePath,
            method,
            length.isFinite() ? length : unit::ByteLength{},
            0U,
            options.modificationTime().value_or(time::DateTime::now()),
            options.comment().value_or(text::String{}),
            {},
            {},
            false,
            [&](ZipEntryRecord &record) {
                compression::impl::CompressionCrc32 crc;
                unit::ByteLength inputLength;
                const auto begin = _offset;
                auto read = [&]() -> mem::ByteBlock {
                    if (length.isFinite() && inputLength == length) {
                        return {};
                    }
                    const auto remaining = length.isFinite() ? length - inputLength : unit::ByteLength{65536U};
                    const auto result = source->read(std::min(unit::ByteLength{65536U}, remaining));
                    if (result == stream::StreamReadStatus::Timeout) {
                        throw compression::CompressionError{CompressionErrorReason::Timeout, "ZIP input timed out."_el};
                    }
                    if (result == stream::StreamReadStatus::Finished) {
                        if (length.isFinite() && inputLength != length) {
                            throw compression::CompressionError{
                                CompressionErrorReason::LengthMismatch, "ZIP input prefix is truncated."_el};
                        }
                        return {};
                    }
                    inputLength = inputLength.addedOrThrow(result.data().length());
                    crc.update(result.data().span());
                    return result.data();
                };
                auto sink = [&](mem::ConstByteSpan bytes) {
                    auto block = mem::ByteBlockEditor{};
                    block.append(bytes);
                    write(block);
                };
                auto transfer =
                    CompressionTransferOptions{}
                        .setMaximumBufferedLength(_options.maximumBufferedItemLength())
                        .setProgress([&](const CompressionProgress &value) {
                            if (!_options.progress() || value.phase == CompressionPhase::Completed) {
                                return CompressionProgressAction::Continue;
                            }
                            return _options.progress()(ZipProgress{value, record.path, ZipOperationPhase::Compression});
                        });
                if (length.isFinite()) {
                    transfer.setInputLength(length);
                }
                if (method == CompressionMethod::Stored) {
                    CompressionProgress progress;
                    progress.inputTotal = transfer.inputLength();
                    const auto notify = [&]() {
                        if (transfer.progress() && transfer.progress()(progress) == CompressionProgressAction::Cancel &&
                            progress.phase != CompressionPhase::Completed) {
                            throw compression::CompressionError{
                                CompressionErrorReason::Cancelled, "ZIP transfer cancelled."_el};
                        }
                    };
                    notify();
                    while (true) {
                        const auto block = read();
                        if (block.isEmpty()) {
                            break;
                        }
                        sink(block.span());
                        progress.transferred = {inputLength, inputLength};
                        notify();
                    }
                    progress.phase = CompressionPhase::Finalizing;
                    notify();
                } else {
                    const auto codec = compression::impl::CompressionCodec::create(
                        zipTools::algorithmForMethod(method), CompressionFormat::Zip, level);

                    codec->compress(read, sink, transfer, {});
                }
                if (!source->close().isClosed()) {
                    throw compression::CompressionError{
                        CompressionErrorReason::Timeout, "ZIP source close timed out."_el};
                }
                record.uncompressedLength = inputLength;
                record.compressedLength = _offset - begin;
                record.crc32 = crc.value();
            },
            bound);
        if (_options.progress()) {
            const auto &record = _entries.back().record;
            _options.progress()(ZipProgress{
                CompressionProgress{
                    {record.uncompressedLength, record.compressedLength},
                    length.isFinite() ? std::optional<unit::ByteLength>{length} : std::nullopt,
                    {},
                    CompressionPhase::Completed},
                record.path,
                ZipOperationPhase::Compression});
        }
    } catch (...) {
        if (_offset != entryOffset) {
            abort();
        }
        source->abort();
        throw;
    }
}

}
