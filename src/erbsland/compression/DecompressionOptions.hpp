// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteCount.hpp"

#include <optional>

namespace erbsland::compression {

/// Limits and validation options for one byte decompressor.
/// Workspace covers dynamically allocated codec dictionaries, tables, indexes, and scratch buffers. It excludes the
/// compressed input, returned output, stack storage, and allocator metadata.
/// @seedoc{/reference/compression/data_compression}
/// @tested{ByteCompressionTest}
class DecompressionOptions final {
public:
    /// The default maximum decompressed output length.
    static constexpr auto cDefaultMaximumOutputLength = unit::ByteCount{256U * 1024U * 1024U};
    /// The default maximum codec workspace length.
    static constexpr auto cDefaultMaximumWorkspaceLength = unit::ByteCount{64U * 1024U * 1024U};

public: // accessors
    /// Get the optional exact decompressed output length.
    [[nodiscard]] constexpr auto expectedOutputLength() const noexcept -> const std::optional<unit::ByteCount> & {
        return _expectedOutputLength;
    }
    /// Set the exact decompressed output length to validate.
    constexpr auto setExpectedOutputLength(unit::ByteCount value) noexcept -> DecompressionOptions & {
        _expectedOutputLength = value;
        return *this;
    }
    /// Clear exact output-length validation.
    constexpr auto clearExpectedOutputLength() noexcept -> DecompressionOptions & {
        _expectedOutputLength.reset();
        return *this;
    }
    /// Get the maximum decompressed output length.
    [[nodiscard]] constexpr auto maximumOutputLength() const noexcept -> unit::ByteCount {
        return _maximumOutputLength;
    }
    /// Set the maximum decompressed output length.
    constexpr auto setMaximumOutputLength(unit::ByteCount value) noexcept -> DecompressionOptions & {
        _maximumOutputLength = value;
        return *this;
    }
    /// Get the maximum codec workspace length.
    [[nodiscard]] constexpr auto maximumWorkspaceLength() const noexcept -> unit::ByteCount {
        return _maximumWorkspaceLength;
    }
    /// Set the maximum codec workspace length.
    constexpr auto setMaximumWorkspaceLength(unit::ByteCount value) noexcept -> DecompressionOptions & {
        _maximumWorkspaceLength = value;
        return *this;
    }

private:
    std::optional<unit::ByteCount> _expectedOutputLength;                    ///< Optional exact output length.
    unit::ByteCount _maximumOutputLength{cDefaultMaximumOutputLength};       ///< Maximum returned output length.
    unit::ByteCount _maximumWorkspaceLength{cDefaultMaximumWorkspaceLength}; ///< Maximum codec workspace length.
};

}
