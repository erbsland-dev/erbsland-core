// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ResourceStorageInfo.hpp"

#include "../../text/EncodingError.hpp"
#include "../../text/EncodingMode.hpp"
#include "../../text/StringBomMode.hpp"
#include "../../text/StringDecoder.hpp"
#include "../../text/StringEncoding.hpp"

#include <algorithm>
#include <array>
#include <type_traits>

namespace erbsland::resource::impl {

auto ResourceStorageInfo::parse(const mem::ByteBlockLiteral storedData, const mem::ByteBlockLiteral infoBlock)
    -> std::optional<ResourceStorageInfo> {
    constexpr auto cHeaderSize = std::size_t{36U};
    constexpr auto cMagic = std::array<mem::Byte, 4U>{
        mem::Byte::fromChar('E'), mem::Byte::fromChar('L'), mem::Byte::fromChar('R'), mem::Byte::fromChar('I')};
    const auto storedBytes = storedData.span();
    const auto infoBytes = infoBlock.span();
    if (infoBytes.size() < cHeaderSize || !std::equal(cMagic.begin(), cMagic.end(), infoBytes.begin())) {
        return std::nullopt;
    }
    const auto version = infoBytes[4U].toUInt8();
    const auto compressionId = infoBytes[5U].toUInt8();
    const auto hashId = infoBytes[6U].toUInt8();
    const auto encryptionId = infoBytes[7U].toUInt8();
    const auto originalSize = decodeInteger<std::uint64_t>(infoBytes, 8U);
    const auto declaredStoredSize = decodeInteger<std::uint64_t>(infoBytes, 16U);
    const auto identifierSize = decodeInteger<std::uint32_t>(infoBytes, 24U);
    const auto pathSize = decodeInteger<std::uint32_t>(infoBytes, 28U);
    const auto hashSize = decodeInteger<std::uint16_t>(infoBytes, 32U);
    const auto reserved = decodeInteger<std::uint16_t>(infoBytes, 34U);
    if (version != 1U || encryptionId != 0U || reserved != 0U || identifierSize == 0U || pathSize == 0U ||
        originalSize >= unit::ByteLength::cRawInfinite || declaredStoredSize != storedBytes.size()) {
        return std::nullopt;
    }
    auto compressionAlgorithm = std::optional<compression::CompressionAlgorithm>{};
    if (compressionId == compression::CompressionAlgorithm::Lz4Block) {
        compressionAlgorithm = compression::CompressionAlgorithm::Lz4Block;
    } else if (compressionId != 0U) {
        return std::nullopt;
    }
    if (!compressionAlgorithm.has_value() && originalSize != declaredStoredSize) {
        return std::nullopt;
    }
    auto hashAlgorithm = std::optional<cryptology::HashAlgorithm>{};
    if (hashId == 1U) {
        hashAlgorithm = cryptology::HashAlgorithm::Sha3_256;
    } else if (hashId != 0U) {
        return std::nullopt;
    }
    if ((!hashAlgorithm.has_value() && hashSize != 0U) ||
        (hashAlgorithm.has_value() && hashSize != hashAlgorithm->digestSize().toRawValue())) {
        return std::nullopt;
    }
    auto remaining = infoBytes.size() - cHeaderSize;
    if (identifierSize > remaining) {
        return std::nullopt;
    }
    remaining -= identifierSize;
    if (pathSize > remaining) {
        return std::nullopt;
    }
    remaining -= pathSize;
    if (hashSize != remaining) {
        return std::nullopt;
    }
    try {
        const auto identifierOffset = cHeaderSize;
        const auto pathOffset = identifierOffset + identifierSize;
        const auto hashOffset = pathOffset + pathSize;
        auto result = ResourceStorageInfo{};
        result.identifier = decodeString(infoBytes.subspan(identifierOffset, identifierSize));
        result.path = decodeString(infoBytes.subspan(pathOffset, pathSize));
        result.storedData = mem::ByteBlock{storedData};
        result.originalSize = unit::ByteLength{originalSize};
        result.compressionAlgorithm = compressionAlgorithm;
        result.hashAlgorithm = hashAlgorithm;
        result.hash = mem::ByteBlock{infoBlock}.slice(
            unit::ByteIndex::fromSizeT(hashOffset), unit::ByteLength::fromSizeT(hashSize));
        return result;
    } catch (const text::EncodingError &) {
        return std::nullopt;
    }
}

template <typename T>
auto ResourceStorageInfo::decodeInteger(const mem::ConstByteSpan bytes, const std::size_t offset) noexcept -> T {
    static_assert(std::is_unsigned_v<T>);
    auto result = T{};
    for (auto index = std::size_t{}; index < sizeof(T); ++index) {
        result =
            static_cast<T>(result | static_cast<T>(static_cast<T>(bytes[offset + index].toUInt8()) << (index * 8U)));
    }
    return result;
}

auto ResourceStorageInfo::decodeString(const mem::ConstByteSpan bytes) -> text::String {
    return text::StringDecoder{mem::ByteBlock::fromSpan(bytes)}.decode(
        text::StringEncoding::Utf8, text::StringBomMode::Reject, text::EncodingMode::Strict);
}

}
