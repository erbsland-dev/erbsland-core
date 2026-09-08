// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Buffer input chunks and finalize them as one compressed value.
///
/// `update()` accepts pieces as they arrive. `finalizeWithEnvelope()` compresses
/// the buffered bytes, caches the result, and prevents further updates until
/// `reset()` is called.
void compressStreaming() {
    const auto firstReadings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{48U}, el::Byte{0x18U}}};
    const auto secondReadings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{48U}, el::Byte{0x19U}}};
    auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};

    // Add two spectrometer batches, then produce one envelope.
    compressor.update(firstReadings);
    compressor.update(secondReadings);
    const auto envelope = compressor.finalizeWithEnvelope();

    el::io::printLine("Instrument        : espectrômetro"_el);
    el::io::printLine("Buffered bytes    : "_el, compressor.bufferedLength().toSizeT());
    el::io::printLine("Envelope bytes    : "_el, envelope.length().toSizeT());
    el::io::printLine("Finalized         : "_el, el::BooleanFormat::yesNo(), compressor.isFinalized());
    el::io::printLine(
        "Cached result     : "_el, el::BooleanFormat::yesNo(), compressor.finalizeWithEnvelope() == envelope);
}

}
