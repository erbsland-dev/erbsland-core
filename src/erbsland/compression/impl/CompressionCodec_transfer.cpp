// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionCodec.hpp"

#include "CompressionTransferOperation.hpp"

#include "../../err/OutOfRangeError.hpp"
#include "../../err/ParameterFailure.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteBlockEditor.hpp"
#include "../../text/Literals.hpp"

#include <limits>
#include <utility>

namespace erbsland::compression::impl {

using namespace mem;
using namespace unit;
using namespace text::literals;

auto CompressionCodec::compress(const ByteBlock &data, const CompressionOptions &options) const -> ByteBlock {
    auto output = ByteBlockEditor{};
    auto used = false;
    auto transfer = CompressionTransferOptions{}.setInputLength(data.length());
    // One-shot calls explicitly own complete values and are not constrained by the transfer fallback default.
    transfer.setMaximumBufferedLength(ByteCount{std::numeric_limits<uint64_t>::max() - 1U});
    compress(
        [&]() -> ByteBlock {
            if (used) {
                return {};
            }
            used = true;
            return data;
        },
        [&](const ConstByteSpan bytes) -> void { output.append(bytes); },
        transfer,
        options);
    return output;
}

auto CompressionCodec::decompress(const ByteBlock &data, const DecompressionOptions &options) const -> ByteBlock {
    auto output = ByteBlockEditor{};
    auto used = false;
    auto transfer = CompressionTransferOptions{}.setInputLength(data.length());
    // One-shot calls explicitly own complete values and are not constrained by the transfer fallback default.
    transfer.setMaximumBufferedLength(ByteCount::maximum());
    decompress(
        [&]() -> ByteBlock {
            if (used) {
                return {};
            }
            used = true;
            return data;
        },
        [&](const ConstByteSpan bytes) { output.append(bytes); },
        transfer,
        options);
    return output;
}

auto CompressionCodec::compress(
    CodecReader::Read read,
    CodecOutput::Write write,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const -> CompressionTransferResult {
    return CompressionTransferOperation{*this, false, std::move(read), std::move(write), transfer, {}, options}.run();
}

auto CompressionCodec::decompress(
    CodecReader::Read read,
    CodecOutput::Write write,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const -> CompressionTransferResult {
    return CompressionTransferOperation{*this, true, std::move(read), std::move(write), transfer, options, {}}.run();
}

auto CompressionCodec::availableWorkspace(const CompressionTransferOptions &transfer, const ByteCount configuredLimit)
    -> ByteCount {
    const auto streamBuffers = transfer.bufferLength()
                                   .addedOrThrow(transfer.bufferLength())
                                   .addedOrThrow(transfer.bufferLength())
                                   .addedOrThrow(transfer.bufferLength());
    if (streamBuffers > configuredLimit) {
        throw err::OutOfRangeError{"Transfer buffers exceed the workspace limit."_el};
    }
    const auto result = configuredLimit - streamBuffers;
    if (!result.isFinite()) {
        throw err::ParameterFailure{"Workspace limit must be finite."_el, "options"_el};
    }
    return result;
}

}
