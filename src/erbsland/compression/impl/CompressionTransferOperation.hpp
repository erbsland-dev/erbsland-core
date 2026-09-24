// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CompressionCodec.hpp"
#include "CompressionCrc32.hpp"

namespace erbsland::compression::impl {

/// Per-call transfer state, progress accounting, and Core envelope handling.
/// @tested{CompressionStreamingTest ByteCompressionTest}
class CompressionTransferOperation final {
public:
    /// Borrow codec policies and own callbacks for one synchronous operation.
    CompressionTransferOperation(
        const CompressionCodec &codec,
        bool decompress,
        CodecReader::Read read,
        CodecOutput::Write write,
        const CompressionTransferOptions &transfer,
        const DecompressionOptions &decode,
        const CompressionOptions &encode);
    // defaults/deletions
    CompressionTransferOperation(const CompressionTransferOperation &) = delete;
    CompressionTransferOperation(CompressionTransferOperation &&) = delete;
    auto operator=(const CompressionTransferOperation &) -> CompressionTransferOperation & = delete;
    auto operator=(CompressionTransferOperation &&) -> CompressionTransferOperation & = delete;

public:
    /// Transfer and validate one complete representation.
    auto run() -> CompressionTransferResult;

private:
    /// Report current progress and process cancellation.
    void notify();
    /// Read input while accounting for its length and optional checksum.
    auto readInput() -> mem::ByteBlock;
    /// Enforce output limits and emit bounded chunks.
    void writeOutput(mem::ConstByteSpan bytes);
    /// Frame compressed output with a Core header, chunks, and trailer.
    void compressCore();
    /// Validate and decode a framed Core payload and its trailer.
    void decompressCore();

private:
    const CompressionCodec &_codec;              ///< Borrowed immutable codec.
    bool _decompress;                            ///< Transfer direction.
    CodecReader::Read _read;                     ///< Input provider.
    CodecOutput::Write _write;                   ///< Output consumer.
    const CompressionTransferOptions &_transfer; ///< Borrowed transfer policy.
    const DecompressionOptions &_decode;         ///< Borrowed decoder policy.
    const CompressionOptions &_encode;           ///< Borrowed encoder policy.
    bool _coreEnvelope;                          ///< Whether framing and CRC are required.
    CompressionProgress _progress;               ///< Current progress.
    CompressionCrc32 _crc;                       ///< Uncompressed payload checksum.
    uint64_t _uncompressed{};                    ///< Total decoded length.
    CodecReader _source;                         ///< Input adapter.
    CodecOutput::Write _target;                  ///< Output adapter.
};

}
