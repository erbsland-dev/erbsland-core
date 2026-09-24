// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionCodec.hpp"

#include <cstddef>

namespace erbsland::compression::impl {

/// Codec for Zstandard frames and Core-wrapped Zstandard payloads.
/// @tested{CompressionCodecTest ByteCompressionTest}
class ZstandardCodec final : public CompressionCodec {
private:
    static constexpr auto cMaximumBlockSize = std::size_t{128U * 1024U}; ///< Maximum Zstandard block size.
    static constexpr auto cBlockHeaderLength = std::size_t{3U};          ///< Encoded block-header length.
    static constexpr auto cFrameOverhead = std::size_t{18U};             ///< Fixed worst-case frame allowance.

public:
    /// Create a Zstandard codec.
    ZstandardCodec(CompressionFormat format, CompressionLevel level) noexcept : CompressionCodec{format, level} {}

public: // implement CompressionCodec
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm override {
        return CompressionAlgorithm::Zstandard;
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
