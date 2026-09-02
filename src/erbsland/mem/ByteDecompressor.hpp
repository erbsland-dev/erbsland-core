// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"
#include "ByteCompressionAlgorithm.hpp"
#include "ByteDecompressor_fwd.hpp"

#include <cstdint>
#include <optional>

namespace erbsland::mem {

/// Decompress raw algorithm blocks and self-describing Erbsland Core envelopes.
/// A default instance can incrementally decode envelopes. Configure an algorithm to decode raw blocks.
/// @seedoc{/reference/mem/memory_and_byte_data}
/// @tested{ByteCompressionTest}
class ByteDecompressor final {
private:
    /// The selected incremental finalization mode.
    enum class FinalizationMode : uint8_t {
        None,     ///< No finalizer was called.
        Raw,      ///< Raw block finalization was selected.
        Envelope, ///< Envelope finalization was selected.
    };

public:
    /// Create an automatic decompressor for buffered envelopes.
    ByteDecompressor() noexcept = default;
    /// Create a decompressor for raw blocks using an algorithm.
    explicit ByteDecompressor(ByteCompressionAlgorithm algorithm);

    // defaults
    ~ByteDecompressor() = default;
    ByteDecompressor(const ByteDecompressor &) = default;
    ByteDecompressor(ByteDecompressor &&) noexcept = default;
    auto operator=(const ByteDecompressor &) -> ByteDecompressor & = default;
    auto operator=(ByteDecompressor &&) noexcept -> ByteDecompressor & = default;

public: // one-shot decompression
    /// Decompress a borrowed raw block to its exact original length.
    /// @throws err::LogicError If no raw algorithm is configured.
    /// @throws ByteCompressionError If the raw block is malformed or has another output length.
    [[nodiscard]] auto decompress(ConstByteSpan data, unit::ByteLength originalLength) const -> ByteBlock;
    /// Decompress an owning raw block and propagate sensitivity.
    [[nodiscard]] auto decompress(const ByteBlock &data, unit::ByteLength originalLength) const -> ByteBlock;
    /// Automatically decode a self-describing envelope.
    /// @param data The complete envelope.
    /// @param maximumOutputSize The largest accepted original length.
    /// @throws err::OutOfRangeError If the declared output exceeds `maximumOutputSize`.
    /// @throws ByteCompressionError If the envelope or compressed payload is invalid.
    [[nodiscard]] static auto decompressWithEnvelope(
        ConstByteSpan data, unit::ByteLength maximumOutputSize = unit::ByteLength::infinite()) -> ByteBlock;
    /// Automatically decode an owning envelope and propagate sensitivity.
    [[nodiscard]] static auto decompressWithEnvelope(
        const ByteBlock &data, unit::ByteLength maximumOutputSize = unit::ByteLength::infinite()) -> ByteBlock;

public: // incremental decompression
    /// Append borrowed compressed input to the incremental buffer.
    /// @throws err::LogicError If this decompressor is finalized.
    void update(ConstByteSpan data);
    /// Append owning compressed input and propagate its sensitivity mark.
    /// @throws err::LogicError If this decompressor is finalized.
    void update(const ByteBlock &data);
    /// Finalize buffered data as a raw block.
    /// @throws err::LogicError If no algorithm is configured or envelope finalization was selected.
    [[nodiscard]] auto finalize(unit::ByteLength originalLength) -> ByteBlock;
    /// Finalize buffered data as a self-describing envelope.
    /// @throws err::LogicError If raw finalization was selected.
    [[nodiscard]] auto finalizeWithEnvelope(unit::ByteLength maximumOutputSize = unit::ByteLength::infinite())
        -> ByteBlock;
    /// Clear buffered input and cached output for reuse.
    void reset() noexcept;

public: // accessors
    /// Get the configured raw algorithm, or no value for automatic envelope-only instances.
    [[nodiscard]] auto algorithm() const noexcept -> const std::optional<ByteCompressionAlgorithm> & {
        return _algorithm;
    }
    /// Test whether incremental input was finalized.
    [[nodiscard]] auto isFinalized() const noexcept -> bool { return _finalizationMode != FinalizationMode::None; }
    /// Get the buffered incremental input length.
    [[nodiscard]] auto bufferedLength() const noexcept -> unit::ByteLength { return _input.length(); }

private:
    /// Decode an envelope and optionally propagate sensitivity.
    [[nodiscard]] static auto decompressEnvelope(ConstByteSpan data, unit::ByteLength maximumOutputSize, bool sensitive)
        -> ByteBlock;
    /// Read a little-endian 64-bit envelope integer.
    [[nodiscard]] static auto readUInt64(ConstByteSpan data, std::size_t offset) noexcept -> uint64_t;
    /// Ensure incremental input remains writable.
    void requireNotFinalized() const;
    /// Ensure a finalization mode is compatible with the cached result.
    void requireFinalizationMode(FinalizationMode mode) const;

private:
    std::optional<ByteCompressionAlgorithm> _algorithm;         ///< Optional raw-block algorithm.
    ByteBlockEditor _input;                                     ///< Buffered incremental input.
    ByteBlock _result;                                          ///< Cached incremental result.
    FinalizationMode _finalizationMode{FinalizationMode::None}; ///< Selected finalization mode.
};

}
