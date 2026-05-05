// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "BestGrowth.hpp"

#include "../../math/SaturatingMath.hpp"

namespace erbsland::mem::impl {

auto bestGrowth(const unit::ByteLength currentAllocationSize, const unit::ByteLength requestedAllocationSize) noexcept
    -> unit::ByteLength {
    if (requestedAllocationSize <= currentAllocationSize) {
        return currentAllocationSize;
    }
    if (requestedAllocationSize < cMinimumGrowthBlock) {
        return cMinimumGrowthBlock;
    }

    const auto requestedSize = requestedAllocationSize.toSizeT();
    const auto maximumBlockSize = cMaximumGrowthBlock.toSizeT();
    if (requestedAllocationSize > cMaximumGrowthBlock) {
        const auto remainder = requestedSize % maximumBlockSize;
        if (remainder == 0U) {
            if (math::willAddOverflow(requestedSize, maximumBlockSize)) {
                return unit::ByteLength::maximum();
            }
            return unit::ByteLength::fromSizeT(requestedSize + maximumBlockSize);
        }
        const auto increment = maximumBlockSize - remainder;
        if (math::willAddOverflow(requestedSize, increment)) {
            return unit::ByteLength::maximum();
        }
        return unit::ByteLength::fromSizeT(requestedSize + increment);
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

}
