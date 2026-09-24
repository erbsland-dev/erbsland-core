// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ByteCompressor.hpp"

#include "impl/CompressionCodec.hpp"

namespace erbsland::compression {

ByteCompressor::ByteCompressor(
    const CompressionAlgorithm algorithm,
    const CompressionFormat format,
    const CompressionLevel level,
    CompressionOptions options) :
    _codec{impl::CompressionCodec::create(algorithm, format, level)}, _options{options} {
}

auto ByteCompressor::compress(const mem::ByteBlock &data) const -> mem::ByteBlock {
    return _codec->compress(data, _options);
}

auto ByteCompressor::maximumCompressedLength(const unit::ByteLength length) const -> unit::ByteLength {
    return _codec->maximumCompressedLength(length);
}

auto ByteCompressor::algorithm() const noexcept -> CompressionAlgorithm {
    return _codec->algorithm();
}

auto ByteCompressor::format() const noexcept -> CompressionFormat {
    return _codec->format();
}

auto ByteCompressor::level() const noexcept -> CompressionLevel {
    return _codec->level();
}

auto ByteCompressor::supportsStreaming() const noexcept -> bool {
    return _codec->supportsStreaming();
}

auto ByteCompressor::compress(
    stream::ByteInputStream &source, stream::ByteOutputStream &destination, CompressionTransferOptions options) const
    -> CompressionTransferResult {
    return _codec->compress(source, destination, options, _options);
}

}
