// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "LzmaCodec.hpp"

#include "LzmaDecoder.hpp"
#include "LzmaEncoder.hpp"

#include "../../../err/OutOfRangeError.hpp"
#include "../../../mem/ByteWriter.hpp"
#include "../../../text/Literals.hpp"
#include "../../CompressionError.hpp"

#include <algorithm>
#include <array>
#include <optional>

namespace erbsland::compression::impl {

using namespace text::literals;

void LzmaCodec::compressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const CompressionOptions &options) const {
    constexpr auto dictionaries = std::array<uint32_t, 5U>{262144U, 2097152U, 8388608U, 16777216U, 33554432U};
    constexpr auto depths = std::array<std::size_t, 5U>{4U, 16U, 64U, 128U, 256U};
    const auto levelIndex = static_cast<std::size_t>(level());
    const auto workspace = uint64_t{dictionaries[levelIndex]} * 7U + 2U * 1024U * 1024U;
    if (unit::ByteLength{workspace} > availableWorkspace(transfer, options.maximumWorkspaceLength())) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    auto header = mem::ByteWriter{};
    if (payloadFormat() == CompressionFormat::Zip) {
        header.writeUInt8(9U).writeUInt8(4U).writeUInt16(5U);
    }
    header.writeUInt8(0x5dU).writeUInt32(dictionaries[levelIndex]);
    if (payloadFormat() == CompressionFormat::Raw) {
        header.writeUInt64(~uint64_t{});
    }
    output(header.toByteBlock().span());
    LzmaEncoder{dictionaries[levelIndex], depths[levelIndex]}.encodeStream(input, output);
}

void LzmaCodec::decompressPayload(
    CodecReader &input,
    const CodecOutput::Write &output,
    const CompressionTransferOptions &transfer,
    const DecompressionOptions &options) const {
    const auto workspace = availableWorkspace(transfer, options.maximumWorkspaceLength());
    constexpr auto minimumWorkspace = unit::ByteLength{2U * 1024U * 1024U};
    if (minimumWorkspace > workspace) {
        throw err::OutOfRangeError{"Codec workspace limit exceeded."_el};
    }
    if (payloadFormat() == CompressionFormat::Zip) {
        if (input.little(1U) != 9U || input.little(1U) != 4U || input.little(2U) != 5U) {
            throw CompressionError{CompressionErrorReason::UnsupportedFeature, "Unsupported LZMA ZIP header."_el};
        }
    }
    const auto properties = input.little(1U);
    if (properties >= 225U) {
        throw CompressionError{CompressionErrorReason::MalformedData, "Invalid LZMA properties."_el};
    }
    const auto dictionary = std::max(uint32_t{1U}, static_cast<uint32_t>(input.little(4U)));
    if (unit::ByteLength{uint64_t{dictionary} + 2U * 1024U * 1024U} > workspace) {
        throw err::OutOfRangeError{"LZMA dictionary limit exceeded."_el};
    }
    auto expected = options.expectedOutputLength();
    if (payloadFormat() == CompressionFormat::Raw) {
        const auto size = input.little(8U);
        if (size != ~uint64_t{}) {
            if (expected && expected->toRawValue() != size) {
                throw CompressionError{CompressionErrorReason::LengthMismatch, "LZMA length mismatch."_el};
            }
            expected = unit::ByteLength{size};
        }
    }
    if (expected && *expected > options.maximumOutputLength()) {
        throw err::OutOfRangeError{"LZMA output limit exceeded."_el};
    }
    LzmaDecoder{
        input,
        dictionary,
        static_cast<uint8_t>(properties % 9U),
        static_cast<uint8_t>((properties / 9U) % 5U),
        static_cast<uint8_t>((properties / 9U) / 5U),
        expected ? std::optional<std::size_t>{expected->toSizeTOrThrow()} : std::nullopt,
        options.maximumOutputLength().toSizeTOrThrow(),
        output}
        .decode();
}

auto LzmaCodec::maximumPayloadLength(const unit::ByteLength length) const -> unit::ByteLength {
    if (!length.isFinite()) {
        throw err::OutOfRangeError{"Compression length must be finite."_el};
    }
    return length.addedOrThrow(length).addedOrThrow(
        payloadFormat() == CompressionFormat::Zip ? cZipPayloadOverhead : cRawPayloadOverhead);
}

}
