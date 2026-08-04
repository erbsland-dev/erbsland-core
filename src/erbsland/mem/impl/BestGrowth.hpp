// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BestGrowthStrategy.hpp"

#include "../../unit/ByteLength.hpp"

#include <cstddef>

namespace erbsland::mem::impl {

/// Calculate a suitable allocation size or element capacity for growing storage.
/// @tested{SharedArrayCapacityTest}
class BestGrowth final {
public:
    constexpr static auto cMinimumGrowthBlock = unit::ByteLength{16U};      ///< Smallest compact allocation block.
    constexpr static auto cMaximumGrowthBlock = unit::ByteLength{0x10000U}; ///< Largest compact growth block.
    constexpr static auto cAllocationPageSize = unit::ByteLength{0x1000U};  ///< Page quantum for large allocations.

    /// Create a calculation for element capacities.
    constexpr BestGrowth(const std::size_t currentCapacity, const std::size_t requestedCapacity) noexcept :
        _current{currentCapacity}, _requested{requestedCapacity} {}
    /// Create a calculation for byte lengths.
    constexpr BestGrowth(const unit::ByteLength currentCapacity, const unit::ByteLength requestedCapacity) noexcept :
        _current{currentCapacity.toSizeT()}, _requested{requestedCapacity.toSizeT()} {}

    // defaults
    ~BestGrowth() = default;
    BestGrowth(const BestGrowth &) = default;
    BestGrowth(BestGrowth &&) = default;
    auto operator=(const BestGrowth &) -> BestGrowth & = default;
    auto operator=(BestGrowth &&) -> BestGrowth & = default;

public:
    /// Calculate the allocation target for byte lengths.
    [[nodiscard]] auto bestGrowth(BestGrowthStrategy strategy = BestGrowthStrategy::Compact) const noexcept
        -> unit::ByteLength;
    /// Calculate an element capacity while accounting for the shared-data allocation overhead.
    template <typename tSharedArrayData>
    [[nodiscard]] auto bestGrowth(BestGrowthStrategy strategy = BestGrowthStrategy::Compact) const noexcept
        -> std::size_t;

private:
    /// Align a byte count upwards without overflowing.
    [[nodiscard]] static auto alignUp(std::size_t value, std::size_t alignment) noexcept -> std::size_t;
    /// Calculate a compact allocation target.
    [[nodiscard]] static auto compactGrowth(unit::ByteLength requestedAllocationSize) noexcept -> unit::ByteLength;
    /// Calculate a geometric allocation target.
    [[nodiscard]] static auto geometricGrowth(
        unit::ByteLength currentAllocationSize, unit::ByteLength requestedAllocationSize) noexcept -> unit::ByteLength;

private:
    std::size_t _current;   ///< Current byte length or element capacity.
    std::size_t _requested; ///< Requested byte length or element capacity.
};

/// Calculate an element capacity while accounting for the shared-data allocation overhead.
template <typename tSharedArrayData>
auto BestGrowth::bestGrowth(const BestGrowthStrategy strategy) const noexcept -> std::size_t {
    using Data = tSharedArrayData;
    if (_requested <= _current) {
        return _current;
    }
    if (!Data::canAllocateWithCapacity(_requested)) {
        return _requested;
    }

    const auto currentAllocationSize = unit::ByteLength::fromSizeT(Data::allocationSizeForCapacity(_current));
    const auto requestedAllocationSize = unit::ByteLength::fromSizeT(Data::allocationSizeForCapacity(_requested));
    const auto targetAllocationSize = BestGrowth{currentAllocationSize, requestedAllocationSize}.bestGrowth(strategy);
    const auto overhead = Data::allocationOverhead();
    if (targetAllocationSize.toSizeT() <= overhead) {
        return _requested;
    }

    const auto targetCapacity = (targetAllocationSize.toSizeT() - overhead) / sizeof(typename Data::DataType);
    if (targetCapacity < _requested || !Data::canAllocateWithCapacity(targetCapacity)) {
        return _requested;
    }
    return targetCapacity;
}

}
