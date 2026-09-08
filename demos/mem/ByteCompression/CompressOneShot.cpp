// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Compress complete bytes as a raw LZ4 block or a self-describing envelope.
///
/// Raw output is compact when a surrounding format stores the algorithm and
/// original length. An envelope carries that metadata with its payload.
void compressOneShot() {
    const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
    const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};

    // Compress one complete instrument sample in both representations.
    const auto rawBlock = compressor.compress(samples);
    const auto envelope = compressor.compressWithEnvelope(samples);

    el::io::printLine("Instrument        : telescópio"_el);
    el::io::printLine("Algorithm         : "_el, compressor.algorithm().toString());
    el::io::printLine("Original bytes    : "_el, samples.length().toSizeT());
    el::io::printLine("Raw block bytes   : "_el, rawBlock.length().toSizeT());
    el::io::printLine("Envelope bytes    : "_el, envelope.length().toSizeT());
}

}
