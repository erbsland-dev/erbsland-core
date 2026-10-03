// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>
#include <erbsland/BooleanFormat.hpp>
#include <erbsland/Byte.hpp>
#include <erbsland/ByteBlock.hpp>
#include <erbsland/ByteBlockEditor.hpp>
#include <erbsland/ByteBlockInputStream.hpp>
#include <erbsland/ByteCompressor.hpp>
#include <erbsland/ByteCount.hpp>
#include <erbsland/ByteDecompressor.hpp>
#include <erbsland/CompressionAlgorithm.hpp>
#include <erbsland/CompressionFormat.hpp>
#include <erbsland/DecompressionOptions.hpp>
#include <erbsland/Path.hpp>
#include <erbsland/PathOperations.hpp>
#include <erbsland/StandardStreams.hpp>
#include <erbsland/TempByteOutputStream.hpp>

namespace demo {

/// Decompress a framed instrument record directly into a byte stream.
void decompressStreaming() {
    const auto readings = el::ByteBlock{el::ByteBlockEditor{el::ByteCount{96U}, el::Byte{0x19U}}};
    const auto encoded =
        el::ByteCompressor{el::CompressionAlgorithm::Deflate, el::CompressionFormat::Core}.compress(readings);
    auto source = el::stream::ByteBlockInputStream{encoded};
    auto destination = el::Path::systemTempDirectoryOrThrow().operations().openTempByteOutputStreamOrThrow();
    const auto limits = el::DecompressionOptions{}.setMaximumOutputLength(el::ByteCount{1024U});
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
