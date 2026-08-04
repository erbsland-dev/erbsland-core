// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ByteDataView.hpp"
#include "UnsafeByteBlockAccess_fwd.hpp"

#include "../ByteBlock.hpp"

namespace erbsland::mem::impl {

/// Gives low-level implementations explicit borrowed access to read-only byte-block storage.
/// Returned views become invalid when the source is modified or destroyed.
/// @warning Never retain or expose the returned view beyond the immediate low-level operation.
/// @tested{ByteBlockTest}
class UnsafeByteBlockAccess final {
public:
    /// Create scoped read-only access to a byte block.
    explicit UnsafeByteBlockAccess(const ByteBlock &block) noexcept;

    // defaults/deletions
    UnsafeByteBlockAccess() = delete;
    ~UnsafeByteBlockAccess() = default;
    UnsafeByteBlockAccess(const UnsafeByteBlockAccess &) = default;
    UnsafeByteBlockAccess(UnsafeByteBlockAccess &&) noexcept = default;
    auto operator=(const UnsafeByteBlockAccess &) -> UnsafeByteBlockAccess & = delete;
    auto operator=(UnsafeByteBlockAccess &&) -> UnsafeByteBlockAccess & = delete;

public:
    /// Access the visible bytes without shared-storage traversal in subsequent operations.
    [[nodiscard]] auto dataView() const noexcept -> ByteDataView;

private:
    const ByteBlock &_block; ///< The borrowed read-only byte block.
};

}
