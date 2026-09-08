// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

namespace demo {

/// Buffer compressed chunks before decoding a raw block or envelope.
///
/// A configured decompressor finalizes raw data with the exact output length.
/// A default decompressor can collect an envelope and determine its algorithm
/// and output length during finalization.
void decompressStreaming() {
    const auto samples = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x19U}}};
    const auto compressor = el::ByteCompressor{el::ByteCompressionAlgorithm::Lz4Block};
    const auto rawBlock = compressor.compress(samples);
    const auto envelope = compressor.compressWithEnvelope(samples);

    // Feed a raw block in two chunks and finalize with its external length.
    const auto rawSplit = rawBlock.length().toSizeT() / 2U;
    auto rawDecompressor = el::ByteDecompressor{el::ByteCompressionAlgorithm::Lz4Block};
    rawDecompressor.update(rawBlock.span().first(rawSplit));
    rawDecompressor.update(rawBlock.span().subspan(rawSplit));
    const auto fromRaw = rawDecompressor.finalize(samples.length());

    // Feed an envelope in two chunks and let the frame select the algorithm.
    const auto envelopeSplit = envelope.length().toSizeT() / 2U;
    auto envelopeDecompressor = el::ByteDecompressor{};
    envelopeDecompressor.update(envelope.span().first(envelopeSplit));
    envelopeDecompressor.update(envelope.span().subspan(envelopeSplit));
    const auto fromEnvelope = envelopeDecompressor.finalizeWithEnvelope(el::ByteLength{1024U});

    el::io::printLine("Instrument        : espectrômetro"_el);
    el::io::printLine("Raw restored      : "_el, el::BooleanFormat::yesNo(), fromRaw == samples);
    el::io::printLine("Envelope restored : "_el, el::BooleanFormat::yesNo(), fromEnvelope == samples);
}

}
