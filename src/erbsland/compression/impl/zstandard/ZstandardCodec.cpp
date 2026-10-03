// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ZstandardCodec.hpp"

#include "ZstandardFrame.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../err/OutOfRangeFailure.hpp"
#include "../../../text/Literals.hpp"

#include <array>

namespace erbsland::compression::impl {

using namespace unit;
using namespace text::literals;

void ZstandardCodec::compressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const {
    constexpr auto windowBits = std::array<unsigned, 5U>{17U, 19U, 21U, 23U, 25U};
    const auto window =
        ByteCount{uint64_t{1U} << windowBits[static_cast<std::size_t>(level())], ByteCount::Unchecked{}};
    const auto workspace = window * 7U + ByteCount{16U * 1024U * 1024U};
    if (workspace > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    ZstandardFrame{}.compressStream(input, level(), output);
}

void ZstandardCodec::decompressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const {
    auto codecOptions = options;
    const auto workspace = availableWorkspace(transfer, options.maximumWorkspaceLength());
    constexpr auto minimumWorkspace = ByteCount{2U * 1024U * 1024U};
    if (minimumWorkspace > workspace) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    codecOptions.setMaximumWorkspaceLength(workspace);
    ZstandardFrame{}.decompressStream(input, codecOptions, output);
}

auto ZstandardCodec::maximumPayloadLength(const ByteCount length) const -> ByteCount {
    if (!length.isFinite()) {
        throw err::OutOfRangeFailure{"Compression length must be finite."_el};
    }
    const auto value = length.toSizeTOrThrow();
    return length.addedOrThrow(
        ByteCount::fromSizeT((value / cMaximumBlockSize + 1U) * cBlockHeaderLength + cFrameOverhead));
}

}
