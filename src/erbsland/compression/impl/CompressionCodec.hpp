// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CodecOutput.hpp"
#include "CodecReader.hpp"
#include "CompressionCodec_fwd.hpp"

#include "../CompressionAlgorithm.hpp"
#include "../CompressionFormat.hpp"
#include "../CompressionLevel.hpp"
#include "../CompressionOptions.hpp"
#include "../CompressionTransferOptions.hpp"
#include "../CompressionTransferResult.hpp"
#include "../DecompressionOptions.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../stream/ByteInputStream_fwd.hpp"
#include "../../stream/ByteOutputStream_fwd.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::compression::impl {

/// Common interface and representation handling for compression algorithms.
/// @tested{CompressionCodecTest ByteCompressionTest}
class CompressionCodec {
    friend class CompressionTransferOperation;

public:
    // defaults
    virtual ~CompressionCodec() = default;

public:
    /// Compress a complete byte value in the configured representation.
    [[nodiscard]] auto compress(const mem::ByteBlock &data, const CompressionOptions &options = {}) const
        -> mem::ByteBlock;
    /// Decompress a complete byte value in the configured representation.
    [[nodiscard]] auto decompress(const mem::ByteBlock &data, const DecompressionOptions &options) const
        -> mem::ByteBlock;
    /// Compress through synchronous bounded callbacks.
    auto compress(
        CodecReader::Read read,
        CodecOutput::Write write,
        const CompressionTransferOptions &transfer,
        const CompressionOptions &options) const -> CompressionTransferResult;
    /// Decompress through synchronous bounded callbacks.
    auto decompress(
        CodecReader::Read read,
        CodecOutput::Write write,
        const CompressionTransferOptions &transfer,
        const DecompressionOptions &options) const -> CompressionTransferResult;
    /// Compress between borrowed streams; neither stream is closed or flushed.
    auto compress(
        stream::ByteInputStream &source,
        stream::ByteOutputStream &destination,
        CompressionTransferOptions transfer,
        const CompressionOptions &options) const -> CompressionTransferResult;
    /// Decompress between borrowed streams; neither stream is closed or flushed.
    auto decompress(
        stream::ByteInputStream &source,
        stream::ByteOutputStream &destination,
        CompressionTransferOptions transfer,
        const DecompressionOptions &options) const -> CompressionTransferResult;
    /// Calculate the worst-case compressed length for the configured representation.
    [[nodiscard]] auto maximumCompressedLength(unit::ByteLength length) const -> unit::ByteLength;
    /// Get the compression algorithm implemented by this codec.
    [[nodiscard]] virtual auto algorithm() const noexcept -> CompressionAlgorithm = 0;
    /// Get the selected compressed representation.
    [[nodiscard]] auto format() const noexcept -> CompressionFormat { return _format; }
    /// Get the selected compression effort level.
    [[nodiscard]] auto level() const noexcept -> CompressionLevel { return _level; }
    /// Test whether this codec uses bounded native streaming.
    [[nodiscard]] virtual auto supportsStreaming() const noexcept -> bool { return true; }
    /// Create a codec for the selected configuration.
    /// @throws err::ParameterError If a value or combination is unsupported.
    [[nodiscard]] static auto create(
        CompressionAlgorithm algorithm, CompressionFormat format, CompressionLevel level = CompressionLevel::Default)
        -> CompressionCodecPtr;

protected:
    /// Create the common codec state.
    CompressionCodec(CompressionFormat format, CompressionLevel level) noexcept : _format{format}, _level{level} {}
    /// Get the payload representation, mapping Core envelopes to Raw payloads.
    [[nodiscard]] auto payloadFormat() const noexcept -> CompressionFormat;
    /// Calculate workspace available after reserving common transfer buffers.
    [[nodiscard]] static auto availableWorkspace(
        const CompressionTransferOptions &transfer, unit::ByteLength configuredLimit) -> unit::ByteLength;
    /// Compress one native payload.
    virtual void compressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const CompressionOptions &options) const = 0;
    /// Decompress one native payload.
    virtual void decompressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const DecompressionOptions &options) const = 0;
    /// Calculate the worst-case algorithm payload length.
    [[nodiscard]] virtual auto maximumPayloadLength(unit::ByteLength length) const -> unit::ByteLength = 0;

private:
    CompressionFormat _format; ///< Selected representation.
    CompressionLevel _level;   ///< Selected compression effort.
};

}
