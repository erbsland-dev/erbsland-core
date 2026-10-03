// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Bzip2Codec.hpp"

#include "Bzip2Decoder.hpp"
#include "Bzip2Encoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../err/OutOfRangeFailure.hpp"
#include "../../../text/Literals.hpp"

namespace erbsland::compression::impl {

using namespace unit;
using namespace text::literals;

void Bzip2Codec::compressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const {
    constexpr auto workspace = ByteCount{32U * 1024U * 1024U};
    if (workspace > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    Bzip2Encoder{level()}.encodeStream(input, output);
}

void Bzip2Codec::decompressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const {
    constexpr auto workspace = ByteCount{32U * 1024U * 1024U};
    if (workspace > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    Bzip2Decoder{input, options, output}.decode();
}

auto Bzip2Codec::maximumPayloadLength(const ByteCount length) const -> ByteCount {
    if (!length.isFinite()) {
        throw err::OutOfRangeFailure{"Compression length must be finite."_el};
    }
    return length.addedOrThrow(length).addedOrThrow(cMaximumPayloadOverhead);
}

}
