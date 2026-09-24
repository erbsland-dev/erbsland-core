// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionCodec.hpp"

namespace erbsland::compression::impl {

/// Codec for bzip2 streams and Core-wrapped bzip2 payloads.
/// @tested{CompressionCodecTest ByteCompressionTest}
class Bzip2Codec final : public CompressionCodec {
private:
    static constexpr auto cMaximumPayloadOverhead = unit::ByteLength{2048U}; ///< Fixed worst-case payload allowance.

public:
    /// Create a bzip2 codec.
    Bzip2Codec(CompressionFormat format, CompressionLevel level) noexcept : CompressionCodec{format, level} {}

public: // implement CompressionCodec
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm override {
        return CompressionAlgorithm::Bzip2;
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
