// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include <DemoCommon.hpp>

#include <array>

namespace demo {

struct CompressionLevelEntry final {
    el::CompressionLevel level;
    el::StringLiteral name;
};

constexpr auto cCompressionLevels = std::array<CompressionLevelEntry, 5U>{
    CompressionLevelEntry{el::CompressionLevel::Fastest, "Fastest"_el},
    CompressionLevelEntry{el::CompressionLevel::Fast, "Fast"_el},
    CompressionLevelEntry{el::CompressionLevel::Default, "Default"_el},
    CompressionLevelEntry{el::CompressionLevel::High, "High"_el},
    CompressionLevelEntry{el::CompressionLevel::Highest, "Highest"_el},
};

auto createObservatoryReadings() -> el::ByteBlock {
    auto readings = el::ByteBlockEditor{};
    for (auto cycle = 0U; cycle < 8U; ++cycle) {
        readings.append(el::Byte{0x18U}, el::ByteLength{192U});
        readings.append(el::Byte{0x2aU}, el::ByteLength{32U});
        readings.append(el::Byte{0x19U}, el::ByteLength{192U});
        readings.append(el::Byte{0x2aU}, el::ByteLength{32U});
    }
    return el::ByteBlock{readings};
}

/// Compare all portable compression levels for one explicitly selected algorithm.
///
/// A level changes only the compressor's effort. The decoder receives the
/// algorithm and representation, but it does not need the compression level.
void compareCompressionLevels(
    const el::CompressionAlgorithm algorithm,
    const el::CompressionFormat format,
    const el::StringLiteral algorithmName) {
    const auto readings = createObservatoryReadings();
    const auto options = el::DecompressionOptions{}
                             .setExpectedOutputLength(readings.length())
                             .setMaximumOutputLength(el::ByteLength{16U * 1024U})
                             .setMaximumWorkspaceLength(el::ByteLength{64U * 1024U * 1024U});

    el::io::printLine("Instrument        : radiotelescópio"_el);
    el::io::printLine("Algorithm         : "_el, algorithmName);
    el::io::printLine("Original bytes    : "_el, readings.length().toSizeT());

    // Compress and restore the same observation at every portable effort level.
    for (const auto &[level, name] : cCompressionLevels) {
        const auto compressor = el::ByteCompressor{algorithm, format, level};
        const auto compressed = compressor.compress(readings);
        const auto decompressor = el::ByteDecompressor{algorithm, format, options};
        const auto restored = decompressor.decompress(compressed);
        el::io::printLine(
            name,
            " bytes: "_el,
            compressed.length().toSizeT(),
            ", restored: "_el,
            el::BooleanFormat::yesNo(),
            restored == readings);
    }
}

/// Compare the five LZ4 compression levels using raw blocks.
void compareLz4Levels() {
    compareCompressionLevels(el::CompressionAlgorithm::Lz4Block, el::CompressionFormat::Raw, "lz4-block"_el);
}

/// Compare the five Deflate compression levels using RFC 1951 streams.
void compareDeflateLevels() {
    compareCompressionLevels(el::CompressionAlgorithm::Deflate, el::CompressionFormat::Raw, "deflate"_el);
}

/// Compare the five bzip2 compression levels using complete streams.
void compareBzip2Levels() {
    compareCompressionLevels(el::CompressionAlgorithm::Bzip2, el::CompressionFormat::Raw, "bzip2"_el);
}

/// Compare the five LZMA compression levels using LZMA-Alone streams.
void compareLzmaLevels() {
    compareCompressionLevels(el::CompressionAlgorithm::Lzma, el::CompressionFormat::Raw, "lzma"_el);
}

/// Compare the five Zstandard compression levels using standard frames.
void compareZstandardLevels() {
    compareCompressionLevels(el::CompressionAlgorithm::Zstandard, el::CompressionFormat::Raw, "zstandard"_el);
}

}
