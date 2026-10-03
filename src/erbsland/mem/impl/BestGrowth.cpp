// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BestGrowth.hpp"

#include "../../math/SaturatingMath.hpp"

#include <algorithm>

namespace erbsland::mem::impl {

using namespace unit;

auto BestGrowth::alignUp(const std::size_t value, const std::size_t alignment) noexcept -> std::size_t {
    const auto remainder = value % alignment;
    if (remainder == 0U) {
        return value;
    }
    const auto increment = alignment - remainder;
    if (math::willAddOverflow(value, increment)) {
        return ByteCount::maximum().toSizeT();
    }
    return value + increment;
}

auto BestGrowth::compactGrowth(const ByteCount requestedAllocationSize) noexcept -> ByteCount {
    const auto requestedSize = requestedAllocationSize.toSizeT();
    const auto maximumBlockSize = cMaximumGrowthBlock.toSizeT();
    if (requestedAllocationSize > cMaximumGrowthBlock) {
        const auto aligned = alignUp(requestedSize, maximumBlockSize);
        if (aligned == requestedSize) {
            if (math::willAddOverflow(requestedSize, maximumBlockSize)) {
                return ByteCount::maximum();
            }
            return ByteCount::fromSizeT(requestedSize + maximumBlockSize);
        }
        return ByteCount::fromSizeT(aligned);
    }

    auto result = std::size_t{1U};
    while (result <= requestedSize) {
        if (result > ByteCount::maximum().toSizeT() / 2U) {
            return ByteCount::maximum();
        }
        result *= 2U;
    }
    return ByteCount::fromSizeT(result);
}

auto BestGrowth::geometricGrowth(
    const ByteCount currentAllocationSize, const ByteCount requestedAllocationSize) noexcept -> ByteCount {
    const auto pageSize = cAllocationPageSize.toSizeT();
    const auto maximum = ByteCount::maximum().toSizeT();
    const auto requested = alignUp(requestedAllocationSize.toSizeT(), pageSize);
    auto result = std::max(pageSize, alignUp(currentAllocationSize.toSizeT(), pageSize));
    while (result < requested) {
        if (result > maximum / 2U) {
            return ByteCount::maximum();
        }
        result *= 2U;
    }
    return ByteCount::fromSizeT(result);
}

auto BestGrowth::bestGrowth(const BestGrowthStrategy strategy) const noexcept -> ByteCount {
    const auto currentAllocationSize = ByteCount::fromSizeT(_current);
    const auto requestedAllocationSize = ByteCount::fromSizeT(_requested);
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
