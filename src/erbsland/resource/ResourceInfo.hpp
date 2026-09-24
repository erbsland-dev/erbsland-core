// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ResourceInfo_fwd.hpp"

#include "../compression/CompressionAlgorithm.hpp"
#include "../cryptology/HashAlgorithm.hpp"
#include "../mem/ByteBlock.hpp"
#include "../unit/ByteLength.hpp"

#include <optional>

namespace erbsland::resource {

/// Metadata for one compiled resource.
/// @tested{ResourceManagerTest}
class ResourceInfo final {
public:
    /// Create resource metadata.
    ResourceInfo(
        unit::ByteLength originalSize,
        unit::ByteLength storedSize,
        std::optional<compression::CompressionAlgorithm> compressionAlgorithm,
        std::optional<cryptology::HashAlgorithm> hashAlgorithm,
        mem::ByteBlock hash,
        bool encrypted) noexcept;

    // defaults
    ResourceInfo() = default;
    ~ResourceInfo() = default;
    ResourceInfo(const ResourceInfo &) = default;
    ResourceInfo(ResourceInfo &&) noexcept = default;
    auto operator=(const ResourceInfo &) -> ResourceInfo & = default;
    auto operator=(ResourceInfo &&) noexcept -> ResourceInfo & = default;

public: // tests
    /// Test whether the resource is compressed.
    [[nodiscard]] auto isCompressed() const noexcept -> bool { return _compressionAlgorithm.has_value(); }
    /// Test whether a hash is available.
    [[nodiscard]] auto hasHash() const noexcept -> bool { return _hashAlgorithm.has_value(); }
    /// Test whether the resource is encrypted.
    [[nodiscard]] auto isEncrypted() const noexcept -> bool { return _encrypted; }

public: // accessors
    /// Get the original logical byte size.
    [[nodiscard]] auto originalSize() const noexcept -> unit::ByteLength { return _originalSize; }
    /// Get the embedded representation size.
    [[nodiscard]] auto storedSize() const noexcept -> unit::ByteLength { return _storedSize; }
    /// Get the compression algorithm, or no value for uncompressed data.
    [[nodiscard]] auto compressionAlgorithm() const noexcept
        -> const std::optional<compression::CompressionAlgorithm> & {
        return _compressionAlgorithm;
    }
    /// Get the hash algorithm, or no value when hashing was disabled.
    [[nodiscard]] auto hashAlgorithm() const noexcept -> const std::optional<cryptology::HashAlgorithm> & {
        return _hashAlgorithm;
    }
    /// Get the logical-data digest, or an empty block when hashing was disabled.
    [[nodiscard]] auto hash() const noexcept -> const mem::ByteBlock & { return _hash; }

private:
    unit::ByteLength _originalSize;                                         ///< Logical data size.
    unit::ByteLength _storedSize;                                           ///< Embedded representation size.
    std::optional<compression::CompressionAlgorithm> _compressionAlgorithm; ///< Optional compression algorithm.
    std::optional<cryptology::HashAlgorithm> _hashAlgorithm;                ///< Optional hash algorithm.
    mem::ByteBlock _hash;                                                   ///< Logical-data digest.
    bool _encrypted{};                                                      ///< Reserved encryption state.
};

}
