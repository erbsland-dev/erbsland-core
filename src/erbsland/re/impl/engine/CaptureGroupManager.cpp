// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaptureGroupManager.hpp"

#include "CaptureGroupManagerWithAtomic.hpp"

#include "../Limits.hpp"

namespace erbsland::re::impl {

#undef ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER
#define ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(minimumGroupCount)                                                    \
    if (captureGroupCount <= minimumGroupCount) {                                                                      \
        return std::make_unique<CaptureGroupManagerBase<minimumGroupCount>>();                                         \
    }
#undef ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER
#define ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(minimumGroupCount)                                                     \
    if (captureGroupCount <= minimumGroupCount) {                                                                      \
        return std::make_unique<CaptureGroupManagerWithAtomic<minimumGroupCount>>();                                   \
    }

auto CaptureGroupManager::create(const std::size_t captureGroupCount, bool hasAtomicGroups) -> CaptureGroupManagerPtr {
    // Allocation sizing strategy
    // The template parameter `tGroupCount` defines the stored set size (and therefore the stride in
    // `_captureGroupSets`).
    //
    // Atomic manager:
    // - Prefer cache-line-friendly size classes to improve iteration/pruning locality.
    //
    // Non-atomic manager:
    // - Prefer tighter size classes to minimize per-thread memory overhead.

    const auto createAtomicMaximum = [&]() -> CaptureGroupManagerPtr {
        constexpr std::size_t cMaximumGroupCount = limits::maximumCaptureGroupCount + 1U;
        constexpr std::size_t cMaximumAtomicGroupCountClass = ((cMaximumGroupCount + 4U) / 4U) * 4U - 1U;
        return std::make_unique<CaptureGroupManagerWithAtomic<cMaximumAtomicGroupCountClass>>();
    };

    if (hasAtomicGroups) {
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(1);
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(3);
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(7);
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(15);
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(31);
        ERBSLAND_RE_CREATE_ATOMIC_GROUP_MANAGER(63);
        return createAtomicMaximum();
    }

    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(1);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(2);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(3);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(5);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(9);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(17);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(33);
    ERBSLAND_RE_CREATE_REGULAR_GROUP_MANAGER(65);
    return std::make_unique<CaptureGroupManagerBase<limits::maximumCaptureGroupCount + 1>>();
}

}
