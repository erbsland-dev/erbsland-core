// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CompressionCodec.hpp"

#include <cstddef>

namespace erbsland::compression::impl {

/// Codec for Deflate streams and Core-wrapped Deflate payloads.
/// @tested{CompressionCodecTest ByteCompressionTest}
class DeflateCodec final : public CompressionCodec {
private:
    static constexpr auto cStoredBlockMaximum = std::size_t{65'535U}; ///< Maximum bytes in one stored block.
    static constexpr auto cStoredBlockOverhead = std::size_t{5U};     ///< Header bytes per stored block.
    static constexpr auto cStreamOverhead = std::size_t{1U};          ///< Fixed worst-case stream allowance.

public:
    /// Create a Deflate codec.
    DeflateCodec(CompressionFormat format, CompressionLevel level) noexcept : CompressionCodec{format, level} {}

public: // implement CompressionCodec
    [[nodiscard]] auto algorithm() const noexcept -> CompressionAlgorithm override {
        return CompressionAlgorithm::Deflate;
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
