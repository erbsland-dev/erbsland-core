// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Decompress complete raw streams and self-describing Core envelopes safely.
///
/// Stored options validate an exact output length and place independent limits
/// on returned bytes and codec workspace before decoding starts.
void decompressOneShot() {
    const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
    const auto rawBlock =
        el::ByteCompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw}.compress(samples);
    const auto envelope =
        el::ByteCompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Core}.compress(samples);

    // Decode a raw block with external metadata and an envelope automatically.
    const auto rawOptions = el::DecompressionOptions{}
                                .setExpectedOutputLength(samples.length())
                                .setMaximumOutputLength(el::ByteLength{1024U})
                                .setMaximumWorkspaceLength(el::ByteLength{4U * 1024U * 1024U});
    const auto rawDecompressor =
        el::ByteDecompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw, rawOptions};
    const auto coreOptions = el::DecompressionOptions{}
                                 .setMaximumOutputLength(el::ByteLength{1024U})
                                 .setMaximumWorkspaceLength(el::ByteLength{4U * 1024U * 1024U});
    const auto coreDecompressor =
        el::ByteDecompressor{el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Core, coreOptions};
    const auto fromRaw = rawDecompressor.decompress(rawBlock);
    const auto fromEnvelope = coreDecompressor.decompress(envelope);

    el::io::printLine("Instrument        : telescópio"_el);
    el::io::printLine("Output limit      : "_el, coreDecompressor.options().maximumOutputLength().toSizeT());
    el::io::printLine("Workspace limit   : "_el, coreDecompressor.options().maximumWorkspaceLength().toSizeT());
    el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
    el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
}

}
