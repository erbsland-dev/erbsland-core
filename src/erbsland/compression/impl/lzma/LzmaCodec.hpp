// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionCodec.hpp"

namespace erbsland::compression::impl {

/// Codec for LZMA1 streams, ZIP payloads, and Core-wrapped LZMA payloads.
/// @tested{CompressionCodecTest ByteCompressionTest}
class LzmaCodec final : public CompressionCodec {
private:
    static constexpr auto cZipPayloadOverhead = unit::ByteLength{64U}; ///< Worst-case ZIP payload allowance.
    static constexpr auto cRawPayloadOverhead = unit::ByteLength{68U}; ///< Worst-case raw payload allowance.

public:
    /// Create an LZMA codec.
    LzmaCodec(CompressionFormat format, CompressionLevel level) noexcept : CompressionCodec{format, level} {}

public: // implement CompressionCodec
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm override {
        return CompressionAlgorithm::Lzma;
    }

protected: // implement CompressionCodec
    void compressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const CompressionOptions &options) const override;
    void decompressPayload(
        CodecReader &input,
        const CodecOutput::Write &output,
        const CompressionTransferOptions &transfer,
        const DecompressionOptions &options) const override;
    [[nodiscard]] auto maximumPayloadLength(unit::ByteLength length) const -> unit::ByteLength override;
};

}
