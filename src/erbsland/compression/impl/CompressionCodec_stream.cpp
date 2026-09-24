// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionCodec.hpp"

#include "../../stream/ByteInputStream.hpp"
#include "../../stream/ByteOutputStream.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>

namespace erbsland::compression::impl {

using namespace text::literals;

auto CompressionCodec::compress(
    stream::ByteInputStream &source,
    stream::ByteOutputStream &destination,
    CompressionTransferOptions transfer,
    const CompressionOptions &options) const -> CompressionTransferResult {
    transfer.setBufferLength(std::min(transfer.bufferLength(), destination.outputSettings().backBufferLimit()));
    auto remaining = transfer.inputLength();
    return compress(
        [&]() -> mem::ByteBlock {
            if (remaining && remaining->isZero()) {
                return {};
            }
            const auto length = remaining ? std::min(*remaining, transfer.bufferLength()) : transfer.bufferLength();
            const auto result = source.read(length);
            if (result == stream::StreamReadStatus::Timeout) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression input timed out."_el};
            }
            if (result == stream::StreamReadStatus::Finished) {
                return {};
            }
            if (remaining) {
                *remaining -= result.data().length();
            }
            return result.data();
        },
        [&](const mem::ConstByteSpan bytes) -> void {
            if (destination.write(bytes) != stream::StreamWriteStatus::Success) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression output timed out."_el};
            }
        },
        transfer,
        options);
}

auto CompressionCodec::decompress(
    stream::ByteInputStream &source,
    stream::ByteOutputStream &destination,
    CompressionTransferOptions transfer,
    const DecompressionOptions &options) const -> CompressionTransferResult {
    transfer.setBufferLength(std::min(transfer.bufferLength(), destination.outputSettings().backBufferLimit()));
    auto remaining = transfer.inputLength();
    return decompress(
        [&]() -> mem::ByteBlock {
            if (remaining && remaining->isZero()) {
                return {};
            }
            const auto length = remaining ? std::min(*remaining, transfer.bufferLength()) : transfer.bufferLength();
            const auto result = source.read(length);
            if (result == stream::StreamReadStatus::Timeout) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression input timed out."_el};
            }
            if (result == stream::StreamReadStatus::Finished) {
                return {};
            }
            if (remaining) {
                *remaining -= result.data().length();
            }
            return result.data();
        },
        [&](const mem::ConstByteSpan bytes) -> void {
            if (destination.write(bytes) != stream::StreamWriteStatus::Success) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression output timed out."_el};
            }
        },
        transfer,
        options);
}

}
