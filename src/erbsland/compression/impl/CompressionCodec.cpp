// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CompressionCodec.hpp"

#include "bzip2/Bzip2Codec.hpp"
#include "deflate/DeflateCodec.hpp"
#include "lz4/Lz4Codec.hpp"
#include "lzma/LzmaCodec.hpp"
#include "zstandard/ZstandardCodec.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::compression::impl {

using namespace err;
using namespace unit;
using namespace text::literals;

auto CompressionCodec::create(
    const CompressionAlgorithm algorithm, const CompressionFormat format, const CompressionLevel level)
    -> CompressionCodecPtr {
    if (algorithm.toRawValue() < CompressionAlgorithm::Lz4Block ||
        algorithm.toRawValue() > CompressionAlgorithm::Zstandard) {
        throw ParameterFailure{"Compression algorithm is invalid."_el, "algorithm"_el};
    }
    if (format > CompressionFormat::Core) {
        throw ParameterFailure{"Compression format is invalid."_el, "format"_el};
    }
    if (level > CompressionLevel::Highest) {
        throw ParameterFailure{"Compression level is invalid."_el, "level"_el};
    }
    if (format == CompressionFormat::Zip && algorithm == CompressionAlgorithm::Lz4Block) {
        throw ParameterFailure{"LZ4 has no ZIP compression-method representation."_el, "format"_el};
    }
    switch (algorithm.toRawValue()) {
    case CompressionAlgorithm::Lz4Block:
        return std::make_shared<Lz4Codec>(format, level);
    case CompressionAlgorithm::Deflate:
        return std::make_shared<DeflateCodec>(format, level);
    case CompressionAlgorithm::Bzip2:
        return std::make_shared<Bzip2Codec>(format, level);
    case CompressionAlgorithm::Lzma:
        return std::make_shared<LzmaCodec>(format, level);
    case CompressionAlgorithm::Zstandard:
        return std::make_shared<ZstandardCodec>(format, level);
    }
    throw ParameterFailure{"Compression algorithm is invalid."_el, "algorithm"_el};
}

auto CompressionCodec::maximumCompressedLength(const ByteCount length) const -> ByteCount {
    auto result = maximumPayloadLength(length);
    if (_format == CompressionFormat::Core) {
        auto chunks = result / 65536U;
        if (!(result % 65536U).isZero()) {
            chunks = chunks.addedOrThrow(ByteCount::one());
        }
        result = result.addedOrThrow(chunks * 4U).addedOrThrow(ByteCount{32U});
    }
    return result;
}

auto CompressionCodec::payloadFormat() const noexcept -> CompressionFormat {
    return _format == CompressionFormat::Core ? CompressionFormat::Raw : _format;
}

}
