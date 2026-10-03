// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionCodec.hpp"

#include "../../stream/ByteInputStream.hpp"
#include "../../stream/ByteOutputStream.hpp"
#include "../../text/Literals.hpp"

#include <algorithm>

namespace erbsland::compression::impl {

using namespace mem;
using namespace stream;
using namespace text::literals;

auto CompressionCodec::compress(
    ByteInputStream &source,
    ByteOutputStream &destination,
    CompressionTransferOptions transfer,
    const CompressionOptions &options) const -> CompressionTransferResult {
    transfer.setBufferLength(std::min(transfer.bufferLength(), destination.outputSettings().backBufferLimit()));
    auto remaining = transfer.inputLength();
    return compress(
        [&]() -> ByteBlock {
            if (remaining && remaining->isZero()) {
                return {};
            }
            const auto length = remaining ? std::min(*remaining, transfer.bufferLength()) : transfer.bufferLength();
            const auto result = source.read(length);
            if (result == StreamReadStatus::Timeout) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression input timed out."_el};
            }
            if (result == StreamReadStatus::Finished) {
                return {};
            }
            if (remaining) {
                *remaining -= result.data().length();
            }
            return result.data();
        },
        [&](const ConstByteSpan bytes) -> void {
            if (destination.write(bytes) != StreamWriteStatus::Success) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression output timed out."_el};
            }
        },
        transfer,
        options);
}

auto CompressionCodec::decompress(
    ByteInputStream &source,
    ByteOutputStream &destination,
    CompressionTransferOptions transfer,
    const DecompressionOptions &options) const -> CompressionTransferResult {
    transfer.setBufferLength(std::min(transfer.bufferLength(), destination.outputSettings().backBufferLimit()));
    auto remaining = transfer.inputLength();
    return decompress(
        [&]() -> ByteBlock {
            if (remaining && remaining->isZero()) {
                return {};
            }
            const auto length = remaining ? std::min(*remaining, transfer.bufferLength()) : transfer.bufferLength();
            const auto result = source.read(length);
            if (result == StreamReadStatus::Timeout) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression input timed out."_el};
            }
            if (result == StreamReadStatus::Finished) {
                return {};
            }
            if (remaining) {
                *remaining -= result.data().length();
            }
            return result.data();
        },
        [&](const ConstByteSpan bytes) -> void {
            if (destination.write(bytes) != StreamWriteStatus::Success) {
                throw CompressionError{CompressionErrorReason::Timeout, "Compression output timed out."_el};
            }
        },
        transfer,
        options);
}

}
