// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupSet.hpp"
#include "CaptureGroupTypes.hpp"

namespace erbsland::re::impl {

/// An extension of the capture group set that contains an atomic group mask.
/// @tparam tGroupCount The maximum number of capture groups this set can handle.
template <std::size_t tGroupCount>
class CaptureGroupSetWithAtomic : public CaptureGroupSet<tGroupCount> {
    using Base = CaptureGroupSet<tGroupCount>;

public:
    /// Create an empty capture-group set.
    explicit CaptureGroupSetWithAtomic() = default;

public:
    /// Get the atomic group mask.
    auto atomicGroupMask() const noexcept -> AtomicGroupMask { return _atomicGroupMask; }
    /// Set the atomic group mask.
    void setAtomicGroupMask(const AtomicGroupMask mask) noexcept { _atomicGroupMask = mask; }
    /// Get a unique ID for each parallel run of a program.
    auto atomicGroupRunId() const noexcept -> AtomicGroupRunId {
        return static_cast<AtomicGroupRunId>(Base::_ranges[0].begin());
    }

private:
    /// The atomic group mask for this group.
    /// Each group has its own bit in this mask.
    /// A group inherits all bits from surrounding groups.
    AtomicGroupMask _atomicGroupMask{cNoAtomicGroupMask};
};

}
