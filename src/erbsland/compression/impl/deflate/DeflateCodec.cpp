// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "DeflateCodec.hpp"

#include "DeflateDecoder.hpp"
#include "DeflateEncoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../err/OutOfRangeFailure.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::compression::impl {

using namespace unit;
using namespace text::literals;

void DeflateCodec::compressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const {
    constexpr auto workspace = ByteCount{2U * 1024U * 1024U};
    if (workspace > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    DeflateEncoder{level()}.encodeStream(input, output);
}

void DeflateCodec::decompressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const {
    constexpr auto workspace = ByteCount{2U * 1024U * 1024U};
    if (workspace > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    DeflateDecoder{input, options, output}.decode();
}

auto DeflateCodec::maximumPayloadLength(const ByteCount length) const -> ByteCount {
    if (!length.isFinite()) {
        throw err::OutOfRangeFailure{"Compression length must be finite."_el};
    }
    const auto value = length.toSizeTOrThrow();
    return length.addedOrThrow(
        ByteCount::fromSizeT((value / cStoredBlockMaximum + 1U) * cStoredBlockOverhead + cStreamOverhead));
}

}
