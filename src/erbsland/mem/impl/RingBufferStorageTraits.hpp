// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../Byte.hpp"

#include <cstddef>
#include <limits>

namespace erbsland::mem::impl {

/// Allocation traits for applying the shared growth policy to byte-vector ring storage.
/// @tested{RingBufferTest}
struct RingBufferStorageTraits final {
    using DataType = Byte;

    /// Test if the requested capacity can be represented as an allocation size.
    [[nodiscard]] static constexpr auto canAllocateWithCapacity(const std::size_t capacity) noexcept -> bool {
        return capacity <= std::numeric_limits<std::size_t>::max() / sizeof(DataType);
    }
    /// Calculate the allocation size for a byte capacity.
    [[nodiscard]] static constexpr auto allocationSizeForCapacity(const std::size_t capacity) noexcept -> std::size_t {
        return capacity * sizeof(DataType);
    }
    /// Get the storage overhead before the first byte.
    [[nodiscard]] static constexpr auto allocationOverhead() noexcept -> std::size_t { return 0U; }
};

}
