// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/stream/ByteBlockInputStream.hpp>
#include <erbsland/stream/TempByteOutputStream.hpp>

namespace demo {
/// Decompress a framed instrument record directly into a byte stream.
void decompressStreaming() {
    const auto readings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x19U}}};
    const auto encoded =
        el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core}.compress(readings);
    auto source = el::stream::ByteBlockInputStream{encoded};
    auto destination = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
    const auto limits = el::DecompressionOptions{}.setMaximumOutputLength(el::ByteLength{1024U});
    const auto decompressor =
        el::ByteDecompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core, limits};
    const auto counts = decompressor.decompress(source, *destination);
    el::io::printLine("Instrument        : espectrômetro"_el);
    el::io::printLine("Core bytes        : "_el, counts.inputLength.toSizeT());
    el::io::printLine("Restored bytes    : "_el, counts.outputLength.toSizeT());
    el::io::printLine("Expected length   : "_el, el::BooleanFormat::yesNo(), counts.outputLength == readings.length());
    if (!destination->close().isClosed()) {
        destination->abort();
    }
}
}
