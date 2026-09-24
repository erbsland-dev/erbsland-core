// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteDecompressor.hpp"

#include "impl/CompressionCodec.hpp"

#include "../err/ParameterError.hpp"
#include "../text/Literals.hpp"

#include <utility>

namespace erbsland::compression {

using namespace text::literals;

ByteDecompressor::ByteDecompressor(
    const CompressionAlgorithm algorithm, const CompressionFormat format, DecompressionOptions options) :
    _codec{impl::CompressionCodec::create(algorithm, format)}, _options{std::move(options)} {
    if (!_options.maximumOutputLength().isFinite() || !_options.maximumWorkspaceLength().isFinite()) {
        throw err::ParameterError{"Decompression limits must be finite."_el, "options"_el};
    }
    if (_options.expectedOutputLength().has_value() &&
        (!_options.expectedOutputLength()->isFinite() ||
            *_options.expectedOutputLength() > _options.maximumOutputLength())) {
        throw err::ParameterError{
            "Expected output length must be finite and within the output limit."_el, "options"_el};
    }
    if (format == CompressionFormat::Raw && algorithm == CompressionAlgorithm::Lz4Block &&
        !_options.expectedOutputLength().has_value()) {
        throw err::ParameterError{"Raw LZ4 decompression requires an expected output length."_el, "options"_el};
    }
}

auto ByteDecompressor::decompress(const mem::ByteBlock &data) const -> mem::ByteBlock {
    return _codec->decompress(data, _options);
}

auto ByteDecompressor::algorithm() const noexcept -> CompressionAlgorithm {
    return _codec->algorithm();
}

auto ByteDecompressor::format() const noexcept -> CompressionFormat {
    return _codec->format();
}

auto ByteDecompressor::supportsStreaming() const noexcept -> bool {
    return _codec->supportsStreaming();
}

auto ByteDecompressor::decompress(
    stream::ByteInputStream &source, stream::ByteOutputStream &destination, CompressionTransferOptions options) const
    -> CompressionTransferResult {
    return _codec->decompress(source, destination, options, _options);
}

}
