// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../unit/ByteLength.hpp"

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
    static constexpr auto cDefaultMaximumOutputLength = unit::ByteLength{256U * 1024U * 1024U};
    /// The default maximum codec workspace length.
    static constexpr auto cDefaultMaximumWorkspaceLength = unit::ByteLength{64U * 1024U * 1024U};

public: // accessors
    /// Get the optional exact decompressed output length.
    [[nodiscard]] constexpr auto expectedOutputLength() const noexcept -> const std::optional<unit::ByteLength> & {
        return _expectedOutputLength;
    }
    /// Set the exact decompressed output length to validate.
    constexpr auto setExpectedOutputLength(unit::ByteLength value) noexcept -> DecompressionOptions & {
        _expectedOutputLength = value;
        return *this;
    }
    /// Clear exact output-length validation.
    constexpr auto clearExpectedOutputLength() noexcept -> DecompressionOptions & {
        _expectedOutputLength.reset();
        return *this;
    }
    /// Get the maximum decompressed output length.
    [[nodiscard]] constexpr auto maximumOutputLength() const noexcept -> unit::ByteLength {
        return _maximumOutputLength;
    }
    /// Set the maximum decompressed output length.
    constexpr auto setMaximumOutputLength(unit::ByteLength value) noexcept -> DecompressionOptions & {
        _maximumOutputLength = value;
        return *this;
    }
    /// Get the maximum codec workspace length.
    [[nodiscard]] constexpr auto maximumWorkspaceLength() const noexcept -> unit::ByteLength {
        return _maximumWorkspaceLength;
    }
    /// Set the maximum codec workspace length.
    constexpr auto setMaximumWorkspaceLength(unit::ByteLength value) noexcept -> DecompressionOptions & {
        _maximumWorkspaceLength = value;
        return *this;
    }

private:
    std::optional<unit::ByteLength> _expectedOutputLength;                    ///< Optional exact output length.
    unit::ByteLength _maximumOutputLength{cDefaultMaximumOutputLength};       ///< Maximum returned output length.
    unit::ByteLength _maximumWorkspaceLength{cDefaultMaximumWorkspaceLength}; ///< Maximum codec workspace length.
};

}
