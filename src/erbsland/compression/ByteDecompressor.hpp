// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDecompressor_fwd.hpp"
#include "CompressionAlgorithm.hpp"
#include "CompressionFormat.hpp"
#include "CompressionTransferOptions.hpp"
#include "DecompressionOptions.hpp"

#include "impl/CompressionCodec_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../stream/ByteInputStream.hpp"
#include "../stream/ByteOutputStream.hpp"

namespace erbsland::compression {

/// Decompress complete byte values using one fixed algorithm, representation, and set of limits.
/// Sensitive input flags are ignored; codec storage is not securely erased. Unsuitable for sensitive data.
/// @seedoc{/reference/compression/data_compression}
/// @tested{ByteCompressionTest}
class ByteDecompressor final {
public:
    /// Create a configured byte decompressor.
    /// @throws err::ParameterError If the options or algorithm and format combination are invalid.
    ByteDecompressor(CompressionAlgorithm algorithm, CompressionFormat format, DecompressionOptions options = {});

    // defaults
    ~ByteDecompressor() = default;
    ByteDecompressor(const ByteDecompressor &) = default;
    ByteDecompressor(ByteDecompressor &&) noexcept = default;
    auto operator=(const ByteDecompressor &) -> ByteDecompressor & = default;
    auto operator=(ByteDecompressor &&) noexcept -> ByteDecompressor & = default;

public: // one-shot decompression
    /// Decompress a byte block using the configured representation and limits.
    /// @throws err::OutOfRangeError If an output or workspace limit would be exceeded.
    /// @throws CompressionError If the compressed representation is invalid.
    [[nodiscard]] auto decompress(const mem::ByteBlock &data) const -> mem::ByteBlock;

public: // stream transfer
    /// Transform one source prefix into a borrowed destination; neither stream is closed or flushed.
    /// @throws CompressionError On invalid data, cancellation, or timeout. Output may be partial.
    auto decompress(
        stream::ByteInputStream &source,
        stream::ByteOutputStream &destination,
        CompressionTransferOptions options = {}) const -> CompressionTransferResult;
    /// Test whether this configuration uses bounded native streaming.
    [[nodiscard]] auto supportsStreaming() const noexcept -> bool;

public: // accessors
    /// Get the selected compression algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm;
    /// Get the selected compressed representation.
    [[nodiscard]] auto format() const noexcept -> CompressionFormat;
    /// Get the decompression limits and validation options.
    [[nodiscard]] auto options() const noexcept -> const DecompressionOptions & { return _options; }

private:
    impl::CompressionCodecPtr _codec; ///< Immutable codec configuration.
    DecompressionOptions _options;    ///< Decoder resource and validation policy.
};

}
