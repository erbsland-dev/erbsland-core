// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceStorageRegistration.hpp"

#include "../../cryptology/HashAlgorithm.hpp"
#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteCompressionAlgorithm.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"
#include "../../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::resource::impl {

/// Parsed and cached information for one compiled-resource storage entry.
/// @tested{ResourceManagerTest}
class ResourceStorageInfo final {
public:
    /// Parse and validate a separated data and metadata block.
    [[nodiscard]] static auto parse(std::span<const std::uint8_t> storedData, std::span<const std::uint8_t> infoBlock)
        -> std::optional<ResourceStorageInfo>;

    // defaults
    ResourceStorageInfo() = default;
    ~ResourceStorageInfo() = default;
    ResourceStorageInfo(const ResourceStorageInfo &) = default;
    ResourceStorageInfo(ResourceStorageInfo &&) noexcept = default;
    auto operator=(const ResourceStorageInfo &) -> ResourceStorageInfo & = default;
    auto operator=(ResourceStorageInfo &&) noexcept -> ResourceStorageInfo & = default;

private:
    /// Decode an unsigned little-endian integer from the metadata block.
    template <typename T>
    [[nodiscard]] static auto decodeInteger(std::span<const std::uint8_t> bytes, std::size_t offset) noexcept -> T;
    /// Decode and strictly validate one UTF-8 metadata string.
    [[nodiscard]] static auto decodeString(std::span<const std::uint8_t> bytes) -> text::String;

public:
    text::String identifier;                                           ///< Resource identifier.
    text::String path;                                                 ///< Normalized relative resource path.
    mem::ConstByteSpan storedData;                                     ///< Exact embedded representation.
    unit::ByteLength originalSize;                                     ///< Original logical byte size.
    std::optional<mem::ByteCompressionAlgorithm> compressionAlgorithm; ///< Optional compression algorithm.
    std::optional<cryptology::HashAlgorithm> hashAlgorithm;            ///< Optional logical-data hash algorithm.
    mem::ByteBlock hash;                                               ///< Logical-data digest.
    bool encrypted{};                                                  ///< Reserved encryption state.
};

}
