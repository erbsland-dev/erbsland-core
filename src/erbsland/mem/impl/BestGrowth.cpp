// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BestGrowth.hpp"

#include "../../math/SaturatingMath.hpp"

#include <algorithm>

namespace erbsland::mem::impl {

auto BestGrowth::alignUp(const std::size_t value, const std::size_t alignment) noexcept -> std::size_t {
    const auto remainder = value % alignment;
    if (remainder == 0U) {
        return value;
    }
    const auto increment = alignment - remainder;
    if (math::willAddOverflow(value, increment)) {
        return unit::ByteLength::maximum().toSizeT();
    }
    return value + increment;
}

auto BestGrowth::compactGrowth(const unit::ByteLength requestedAllocationSize) noexcept -> unit::ByteLength {
    const auto requestedSize = requestedAllocationSize.toSizeT();
    const auto maximumBlockSize = cMaximumGrowthBlock.toSizeT();
    if (requestedAllocationSize > cMaximumGrowthBlock) {
        const auto aligned = alignUp(requestedSize, maximumBlockSize);
        if (aligned == requestedSize) {
            if (math::willAddOverflow(requestedSize, maximumBlockSize)) {
                return unit::ByteLength::maximum();
            }
            return unit::ByteLength::fromSizeT(requestedSize + maximumBlockSize);
        }
        return unit::ByteLength::fromSizeT(aligned);
    }

    auto result = std::size_t{1U};
    while (result <= requestedSize) {
        if (result > unit::ByteLength::maximum().toSizeT() / 2U) {
            return unit::ByteLength::maximum();
        }
        result *= 2U;
    }
    return unit::ByteLength::fromSizeT(result);
}

auto BestGrowth::geometricGrowth(
    const unit::ByteLength currentAllocationSize, const unit::ByteLength requestedAllocationSize) noexcept
    -> unit::ByteLength {
    const auto pageSize = cAllocationPageSize.toSizeT();
    const auto maximum = unit::ByteLength::maximum().toSizeT();
    const auto requested = alignUp(requestedAllocationSize.toSizeT(), pageSize);
    auto result = std::max(pageSize, alignUp(currentAllocationSize.toSizeT(), pageSize));
    while (result < requested) {
        if (result > maximum / 2U) {
            return unit::ByteLength::maximum();
        }
        result *= 2U;
    }
    return unit::ByteLength::fromSizeT(result);
}

auto BestGrowth::bestGrowth(const BestGrowthStrategy strategy) const noexcept -> unit::ByteLength {
    const auto currentAllocationSize = unit::ByteLength::fromSizeT(_current);
    const auto requestedAllocationSize = unit::ByteLength::fromSizeT(_requested);
    if (requestedAllocationSize <= currentAllocationSize) {
        return currentAllocationSize;
    }
    if (strategy == BestGrowthStrategy::Geometric) {
        return geometricGrowth(currentAllocationSize, requestedAllocationSize);
    }
    if (requestedAllocationSize < cMinimumGrowthBlock) {
        return cMinimumGrowthBlock;
    }
    return compactGrowth(requestedAllocationSize);
}

}
