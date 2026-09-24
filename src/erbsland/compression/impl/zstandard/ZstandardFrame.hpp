// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CodecOutput.hpp"
#include "../CodecReader.hpp"

#include "../../../mem/ByteBlock_fwd.hpp"
#include "../../../mem/ByteBlockEditor_fwd.hpp"
#include "../../../mem/ByteSpan.hpp"
#include "../../../text/String_fwd.hpp"
#include "../../CompressionLevel.hpp"
#include "../../DecompressionOptions.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace erbsland::compression::impl {

/// Encodes and decodes the supported subset of Zstandard frames.
/// @tested{ByteCompressionTest}
class ZstandardFrame final {
private:
    static constexpr auto cMagic = uint32_t{0xfd2fb528U};                ///< Zstandard frame magic.
    static constexpr auto cMaximumBlockSize = std::size_t{128U * 1024U}; ///< Maximum block size.

private:
    static constexpr auto cSkippableMagicMask = uint32_t{0xfffffff0U}; ///< Mask for skippable-frame magic.
    static constexpr auto cSkippableMagic = uint32_t{0x184d2a50U};     ///< Base skippable-frame magic.
    static constexpr auto cChecksumFlag = uint8_t{0x04U};              ///< Frame descriptor checksum flag.
    static constexpr auto cReservedFlag = uint8_t{0x08U};              ///< Reserved frame descriptor flag.
    static constexpr auto cSingleSegmentFlag = uint8_t{0x20U};         ///< Single-segment frame flag.
    static constexpr auto cDictionaryFlagMask = uint8_t{0x03U};        ///< Dictionary identifier size flag.
    static constexpr auto cWindowLogBase = unsigned{10U};              ///< Minimum encoded window logarithm.
    static constexpr auto cTwoByteContentSizeBase = std::size_t{256U}; ///< Base of two-byte content sizes.
    /// Window logarithm for each compression level.
    inline static constexpr auto cWindowLogs = std::array<unsigned, 5U>{17U, 19U, 21U, 23U, 25U};

public:
    /// Encode a continuous frame from bounded input.
    void compressStream(CodecReader &input, CompressionLevel level, const CodecOutput::Write &output) const;
    /// Decode a continuous frame with bounded history.
    void decompressStream(
        CodecReader &input, const DecompressionOptions &options, const CodecOutput::Write &output) const;
    // defaults
    ZstandardFrame() = default;

private:
    /// Map a compression level to the encoded window-log value.
    [[nodiscard]] static auto windowLogFor(CompressionLevel level) noexcept -> unsigned;
    /// Test whether the complete input consists of one repeated byte.
    [[nodiscard]] static auto isRepeated(mem::ConstByteSpan input) noexcept -> bool;
    /// Append a little-endian integer using the requested byte count.
    static void appendLittle(mem::ByteBlockEditor &output, uint64_t value, std::size_t count);
    /// Throw a malformed-data error with the supplied detail.
    [[noreturn]] static void malformed(const text::String &message);
};

}
