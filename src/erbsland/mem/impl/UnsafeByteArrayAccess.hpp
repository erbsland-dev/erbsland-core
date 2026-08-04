// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "UnsafeByteArrayAccess_fwd.hpp"

#include "../ByteArray.hpp"
#include "../ByteSpan.hpp"

namespace erbsland::mem::impl {

/// Gives low-level implementations explicit writable access to fixed byte-array storage.
/// Returned spans borrow the array and become invalid when it is moved or destroyed.
/// @warning Never retain or expose the returned span beyond the immediate low-level operation.
/// @tested{ByteArrayTest}
template <std::size_t N>
class UnsafeByteArrayAccess final {
public:
    /// Create scoped writable access to an array.
    explicit UnsafeByteArrayAccess(ByteArray<N> &array) noexcept : _array{array} {}

    // defaults/deletions
    UnsafeByteArrayAccess() = delete;
    ~UnsafeByteArrayAccess() = default;
    UnsafeByteArrayAccess(const UnsafeByteArrayAccess &) = default;
    UnsafeByteArrayAccess(UnsafeByteArrayAccess &&) noexcept = default;
    auto operator=(const UnsafeByteArrayAccess &) -> UnsafeByteArrayAccess & = delete;
    auto operator=(UnsafeByteArrayAccess &&) -> UnsafeByteArrayAccess & = delete;

public:
    /// Access the complete writable array storage.
    [[nodiscard]] auto writableData() noexcept -> FixedByteSpan<N> { return FixedByteSpan<N>{_array.writableSpan()}; }

private:
    ByteArray<N> &_array; ///< The borrowed fixed byte array.
};

}
