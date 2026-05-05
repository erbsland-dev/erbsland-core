// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"

#include <cstddef>

namespace erbsland::mem::impl {

/// The smallest allocation block used by compact shared arrays.
constexpr auto cMinimumGrowthBlock = unit::ByteLength{16U};

/// The largest growth block used before switching to fixed-size block rounding.
constexpr auto cMaximumGrowthBlock = unit::ByteLength{0x10000U};

/// Calculate a good allocation size for growing compact shared arrays.
/// @param currentAllocationSize The current total allocation size in bytes.
/// @param requestedAllocationSize The requested total allocation size in bytes.
/// @return The total allocation size that should be allocated.
/// @tested{SharedArrayCapacityTest}
[[nodiscard]] auto bestGrowth(unit::ByteLength currentAllocationSize, unit::ByteLength requestedAllocationSize) noexcept
    -> unit::ByteLength;

/// Calculate a good element capacity for a compact shared array.
/// This helper accounts for the data container overhead before applying the growth policy.
/// @tparam tSharedArrayData The shared array data type.
/// @param currentCapacity The current element capacity.
/// @param requestedCapacity The requested element capacity.
/// @return A capacity that is at least `requestedCapacity`.
/// @tested{SharedArrayCapacityTest}
template <typename tSharedArrayData>
[[nodiscard]] auto bestGrowthCapacity(const std::size_t currentCapacity, const std::size_t requestedCapacity) noexcept
    -> std::size_t {
    using Data = tSharedArrayData;
    if (requestedCapacity <= currentCapacity) {
        return currentCapacity;
    }
    if (!Data::canAllocateWithCapacity(requestedCapacity)) {
        return requestedCapacity;
    }

    const auto currentAllocationSize = unit::ByteLength::fromSizeT(Data::allocationSizeForCapacity(currentCapacity));
    const auto requestedAllocationSize =
        unit::ByteLength::fromSizeT(Data::allocationSizeForCapacity(requestedCapacity));
    const auto targetAllocationSize = bestGrowth(currentAllocationSize, requestedAllocationSize).toSizeT();
    const auto overhead = Data::allocationOverhead();
    if (targetAllocationSize <= overhead) {
        return requestedCapacity;
    }

    const auto targetCapacity = (targetAllocationSize - overhead) / sizeof(typename Data::DataType);
    if (targetCapacity < requestedCapacity || !Data::canAllocateWithCapacity(targetCapacity)) {
        return requestedCapacity;
    }
    return targetCapacity;
}

}
