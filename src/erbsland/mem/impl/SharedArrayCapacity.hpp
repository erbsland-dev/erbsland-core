// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BestGrowth.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace erbsland::mem::impl {

/// Ensure sufficient capacity for a shared array data pointer.
///
/// The callback is called only when a new data block is allocated. It receives the old data pointer, which can be null,
/// and the new data pointer. Callers use it to copy the visible data and maintain container-specific invariants.
///
/// @tparam tSharedArrayDataPointer A `SharedDataPointer<SharedArrayData<...>>` type.
/// @tparam tCopyCallback The callback type.
/// @param data The shared array data pointer to update.
/// @param usedSize The number of used elements in the new data block.
/// @param requiredCapacity The required element capacity.
/// @param forceReallocate If `true`, allocate a new data block even when capacity is already sufficient.
/// @param copyCallback The callback that copies/finalizes data in the new block.
/// @return `true` if a new data block was allocated.
/// @tested{SharedArrayCapacityTest}
template <typename tSharedArrayDataPointer, typename tCopyCallback>
auto ensureSharedArrayCapacity(
    tSharedArrayDataPointer &data,
    const std::size_t usedSize,
    const std::size_t requiredCapacity,
    const bool forceReallocate,
    tCopyCallback &&copyCallback) -> bool {
    using Data = typename tSharedArrayDataPointer::Type;

    const auto oldData = data.constGet();
    const auto oldCapacity = oldData == nullptr ? std::size_t{0} : static_cast<std::size_t>(oldData->capacity());
    if (!forceReallocate && oldData != nullptr && !data.isShared() && requiredCapacity <= oldCapacity) {
        return false;
    }

    const auto requestedCapacity = std::max(usedSize, requiredCapacity);
    const auto newCapacity = BestGrowth{oldCapacity, requestedCapacity}.bestGrowth<Data>();
    auto newData = tSharedArrayDataPointer{Data::create(
        static_cast<typename Data::SizeType>(usedSize), static_cast<typename Data::SizeType>(newCapacity))};
    std::forward<tCopyCallback>(copyCallback)(oldData, newData.get());
    data = std::move(newData);
    return true;
}

}
