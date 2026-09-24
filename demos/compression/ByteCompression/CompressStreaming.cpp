// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/stream/ByteBlockInputStream.hpp>
#include <erbsland/stream/TempByteOutputStream.hpp>

namespace demo {
/// Compress instrument readings directly into a byte stream.
void compressStreaming() {
    const auto readings = el::ByteBlock{el::ByteBlockEditor{el::ByteLength{96U}, el::Byte{0x18U}}};
    auto source = el::stream::ByteBlockInputStream{readings};
    auto destination = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
    const auto compressor = el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core};
    const auto counts = compressor.compress(source, *destination);
    el::io::printLine("Instrument        : espectrômetro"_el);
    el::io::printLine("Input bytes       : "_el, counts.inputLength.toSizeT());
    el::io::printLine("Core bytes        : "_el, counts.outputLength.toSizeT());
    el::io::printLine("Native streaming  : "_el, el::BooleanFormat::yesNo(), compressor.supportsStreaming());
    if (!destination->close().isClosed()) {
        destination->abort();
    }
}
}
