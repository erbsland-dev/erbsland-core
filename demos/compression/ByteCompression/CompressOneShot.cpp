// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Compress complete bytes using an explicitly selected algorithm, representation, and effort.
///
/// The configured compressor can be reused for independent values. Its maximum
/// compressed length includes the framing overhead of the selected format.
void compressOneShot() {
    const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x2aU}}};
    const auto rawCompressor =
        el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Raw, el::CompressionLevel::High};
    const auto coreCompressor =
        el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core, el::CompressionLevel::High};

    // Compress one complete instrument sample in a codec stream and a Core envelope.
    const auto rawStream = rawCompressor.compress(samples);
    const auto coreEnvelope = coreCompressor.compress(samples);
    const auto maximumEnvelopeLength = coreCompressor.maximumCompressedLength(samples.length());

    el::io::printLine("Instrument        : telescópio"_el);
    el::io::printLine("Algorithm         : "_el, rawCompressor.algorithm().toString());
    el::io::printLine("Original bytes    : "_el, samples.length().toSizeT());
    el::io::printLine("Raw stream bytes  : "_el, rawStream.length().toSizeT());
    el::io::printLine("Core bytes        : "_el, coreEnvelope.length().toSizeT());
    el::io::printLine("Core upper bound  : "_el, maximumEnvelopeLength.toSizeT());
}

}
