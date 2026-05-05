// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../IntegerUnit_fwd.hpp"

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace erbsland::unit::impl {

/// A valid index type for use with IntegerUnitIndex.
template <typename tType>
concept ValidIndexType =
    std::is_same_v<tType, uint64_t> || std::is_same_v<tType, uint32_t> || std::is_same_v<tType, uint16_t>;

/// A valid length type for use with IntegerUnitAmount and IntegerUnitRange.
template <typename tType>
concept ValidAmountType =
    std::is_same_v<tType, uint64_t> || std::is_same_v<tType, uint32_t> || std::is_same_v<tType, uint16_t>;

/// A valid offset type for use with IntegerUnitOffset.
template <typename tType>
concept ValidOffsetType = std::is_same_v<tType, int64_t> || std::is_same_v<tType, int32_t>;

/// A valid unit tag type.
template <typename T>
concept ValidIntegerUnit = std::derived_from<T, IntegerUnit> &&
    requires {
        typename T::IndexType;
        typename T::AmountType;
        typename T::OffsetType;
    } && ValidIndexType<typename T::IndexType> && ValidAmountType<typename T::AmountType> &&
    ValidOffsetType<typename T::OffsetType>;

}
