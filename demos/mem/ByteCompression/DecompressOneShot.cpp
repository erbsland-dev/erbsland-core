// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Decompress complete raw blocks and self-describing envelopes.
///
/// Raw decompression needs the algorithm and exact original length. Envelope
/// decompression discovers both from the frame and can enforce an output limit
/// before allocating the result.
void decompressOneShot() {
    const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
    const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};
    const auto rawBlock = compressor.compress(samples);
    const auto envelope = compressor.compressWithEnvelope(samples);

    // Decode a raw block with external metadata and an envelope automatically.
    const auto rawDecompressor = el::ByteDecompressor{el::ByteCompressionAlgorithm::Lz4Block};
    const auto fromRaw = rawDecompressor.decompress(rawBlock, samples.length());
    const auto fromEnvelope = el::ByteDecompressor::decompressWithEnvelope(envelope, el::ByteLength{1024U});

    el::io::printLine("Instrument        : telescópio"_el);
    el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
    el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
}

}
