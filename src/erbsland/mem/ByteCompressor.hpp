// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteBlock.hpp"
#include "ByteBlockEditor.hpp"
#include "ByteCompressionAlgorithm.hpp"
#include "ByteCompressor_fwd.hpp"

#include <cstdint>

namespace erbsland::mem {

/// Compress complete byte values as raw blocks or self-describing envelopes.
/// Incremental input is buffered until either finalization method is called. One-shot calls are independent of the
/// incremental state. After finalization, call `reset()` before adding more input or choosing another output format.
/// @seedoc{/reference/mem/byte_compression}
/// @tested{ByteCompressionTest}
class ByteCompressor final {
private:
    /// The selected incremental finalization mode.
    enum class FinalizationMode : uint8_t {
        None,     ///< No finalizer was called.
        Raw,      ///< Raw block finalization was selected.
        Envelope, ///< Envelope finalization was selected.
    };

public:
    /// Create a compressor for an algorithm.
    explicit ByteCompressor(ByteCompressionAlgorithm algorithm);

    // defaults
    ~ByteCompressor() = default;
    ByteCompressor(const ByteCompressor &) = default;
    ByteCompressor(ByteCompressor &&) noexcept = default;
    auto operator=(const ByteCompressor &) -> ByteCompressor & = default;
    auto operator=(ByteCompressor &&) noexcept -> ByteCompressor & = default;

public: // one-shot compression
    /// Compress borrowed bytes into one raw algorithm block.
    [[nodiscard]] auto compress(ConstByteSpan data) const -> ByteBlock;
    /// Compress an owning byte block and propagate its sensitivity mark.
    [[nodiscard]] auto compress(const ByteBlock &data) const -> ByteBlock;
    /// Compress borrowed bytes into a self-describing Erbsland Core envelope.
    [[nodiscard]] auto compressWithEnvelope(ConstByteSpan data) const -> ByteBlock;
    /// Compress an owning byte block into an envelope and propagate sensitivity.
    [[nodiscard]] auto compressWithEnvelope(const ByteBlock &data) const -> ByteBlock;

public: // incremental compression
    /// Append borrowed input to the incremental buffer.
    /// @throws err::LogicError If this compressor is finalized.
    void update(ConstByteSpan data);
    /// Append an owning input and propagate its sensitivity mark.
    /// @throws err::LogicError If this compressor is finalized.
    void update(const ByteBlock &data);
    /// Finalize incremental input as one raw block.
    /// Repeated calls return the cached result.
    /// @throws err::LogicError If envelope finalization was selected.
    [[nodiscard]] auto finalize() -> ByteBlock;
    /// Finalize incremental input as a self-describing envelope.
    /// Repeated calls return the cached result.
    /// @throws err::LogicError If raw finalization was selected.
    [[nodiscard]] auto finalizeWithEnvelope() -> ByteBlock;
    /// Clear buffered input and cached output for reuse.
    void reset() noexcept;

public: // accessors
    /// Get the selected compression algorithm.
    [[nodiscard]] auto algorithm() const noexcept -> ByteCompressionAlgorithm { return _algorithm; }
    /// Test whether incremental input was finalized.
    [[nodiscard]] auto isFinalized() const noexcept -> bool { return _finalizationMode != FinalizationMode::None; }
    /// Get the buffered incremental input length.
    [[nodiscard]] auto bufferedLength() const noexcept -> unit::ByteLength { return _input.length(); }

private:
    /// Create an envelope around a raw compressed payload.
    [[nodiscard]] auto createEnvelope(
        const ByteBlock &compressed, unit::ByteLength originalLength, bool sensitive) const -> ByteBlock;
    /// Ensure incremental input remains writable.
    void requireNotFinalized() const;
    /// Ensure a finalization mode is compatible with the cached result.
    void requireFinalizationMode(FinalizationMode mode) const;

private:
    ByteCompressionAlgorithm _algorithm;                        ///< The selected compression algorithm.
    ByteBlockEditor _input;                                     ///< Buffered incremental input.
    ByteBlock _result;                                          ///< Cached incremental result.
    FinalizationMode _finalizationMode{FinalizationMode::None}; ///< Selected finalization mode.
};

}
