// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteCompressor_fwd.hpp"
#include "CompressionAlgorithm.hpp"
#include "CompressionFormat.hpp"
#include "CompressionLevel.hpp"
#include "CompressionOptions.hpp"
#include "CompressionTransferOptions.hpp"

#include "impl/CompressionCodec_fwd.hpp"

#include "../mem/ByteBlock.hpp"
#include "../stream/ByteInputStream.hpp"
#include "../stream/ByteOutputStream.hpp"

namespace erbsland::compression {

/// Compress complete byte values using one fixed algorithm, representation, and effort level.
/// Sensitive input flags are ignored; codec storage is not securely erased. Unsuitable for sensitive data.
/// @seedoc{/reference/compression/data_compression}
/// @tested{ByteCompressionTest}
class ByteCompressor final {
public:
    /// Create a configured byte compressor.
    /// @throws err::ParameterError If the algorithm and format combination is unsupported.
    ByteCompressor(
        CompressionAlgorithm algorithm,
        CompressionFormat format,
        CompressionLevel level = CompressionLevel::Default,
        CompressionOptions options = {});

    // defaults
    ~ByteCompressor() = default;
    ByteCompressor(const ByteCompressor &) = default;
    ByteCompressor(ByteCompressor &&) noexcept = default;
    auto operator=(const ByteCompressor &) -> ByteCompressor & = default;
    auto operator=(ByteCompressor &&) noexcept -> ByteCompressor & = default;

public: // one-shot compression
    /// Compress a byte block using the configured representation.
    [[nodiscard]] auto compress(const mem::ByteBlock &data) const -> mem::ByteBlock;
    /// Calculate the upper bound for a compressed result in the configured representation.
    /// @throws err::OutOfRangeError If `length` is infinite or the bound is not representable.
    [[nodiscard]] auto maximumCompressedLength(unit::ByteLength length) const -> unit::ByteLength;

public: // stream transfer
    /// Transform one source prefix into a borrowed destination; neither stream is closed or flushed.
    /// @throws CompressionError On invalid data, cancellation, or timeout. Output may be partial.
    auto compress(
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
    /// Get the selected compression effort level.
    [[nodiscard]] auto level() const noexcept -> CompressionLevel;

private:
    impl::CompressionCodecPtr _codec; ///< Immutable codec configuration.
    CompressionOptions _options;      ///< Compressor workspace policy.
};

}
