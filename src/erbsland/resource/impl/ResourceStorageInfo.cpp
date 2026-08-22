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

auto ResourceStorageInfo::parse(
    const std::span<const std::uint8_t> storedData, const std::span<const std::uint8_t> infoBlock)
    -> std::optional<ResourceStorageInfo> {
    constexpr auto cHeaderSize = std::size_t{36U};
    constexpr auto cMagic = std::array<std::uint8_t, 4U>{'E', 'L', 'R', 'I'};
    if (infoBlock.size() < cHeaderSize || !std::equal(cMagic.begin(), cMagic.end(), infoBlock.begin())) {
        return std::nullopt;
    }
    const auto version = infoBlock[4U];
    const auto compressionId = infoBlock[5U];
    const auto hashId = infoBlock[6U];
    const auto encryptionId = infoBlock[7U];
    const auto originalSize = decodeInteger<std::uint64_t>(infoBlock, 8U);
    const auto declaredStoredSize = decodeInteger<std::uint64_t>(infoBlock, 16U);
    const auto identifierSize = decodeInteger<std::uint32_t>(infoBlock, 24U);
    const auto pathSize = decodeInteger<std::uint32_t>(infoBlock, 28U);
    const auto hashSize = decodeInteger<std::uint16_t>(infoBlock, 32U);
    const auto reserved = decodeInteger<std::uint16_t>(infoBlock, 34U);
    if (version != 1U || encryptionId != 0U || reserved != 0U || identifierSize == 0U || pathSize == 0U ||
        originalSize >= unit::ByteLength::cRawInfinite || declaredStoredSize != storedData.size()) {
        return std::nullopt;
    }
    auto compressionAlgorithm = std::optional<mem::ByteCompressionAlgorithm>{};
    if (compressionId == mem::ByteCompressionAlgorithm::Lz4Block) {
        compressionAlgorithm = mem::ByteCompressionAlgorithm::Lz4Block;
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
    auto remaining = infoBlock.size() - cHeaderSize;
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
        result.identifier = decodeString(infoBlock.subspan(identifierOffset, identifierSize));
        result.path = decodeString(infoBlock.subspan(pathOffset, pathSize));
        result.storedData = mem::toConstByteSpan(storedData);
        result.originalSize = unit::ByteLength{originalSize};
        result.compressionAlgorithm = compressionAlgorithm;
        result.hashAlgorithm = hashAlgorithm;
        result.hash = mem::ByteBlock::fromSpan(infoBlock.subspan(hashOffset, hashSize));
        return result;
    } catch (const text::EncodingError &) {
        return std::nullopt;
    }
}

template <typename T>
auto ResourceStorageInfo::decodeInteger(const std::span<const std::uint8_t> bytes, const std::size_t offset) noexcept
    -> T {
    static_assert(std::is_unsigned_v<T>);
    auto result = T{};
    for (auto index = std::size_t{}; index < sizeof(T); ++index) {
        result = static_cast<T>(result | static_cast<T>(static_cast<T>(bytes[offset + index]) << (index * 8U)));
    }
    return result;
}

auto ResourceStorageInfo::decodeString(const std::span<const std::uint8_t> bytes) -> text::String {
    return text::StringDecoder{mem::ByteBlock::fromSpan(bytes)}.decode(
        text::StringEncoding::Utf8, text::StringBomMode::Reject, text::EncodingMode::Strict);
}

}
