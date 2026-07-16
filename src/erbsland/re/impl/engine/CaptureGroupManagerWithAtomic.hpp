// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManagerBase.hpp"
#include "CaptureGroupSetWithAtomic.hpp"

namespace erbsland::re::impl {

/// The extended implementation of the capture group manager with atomic group id's.
///
/// First, atomic groups are identified by the atomic-group mask, where each atomic group has it's own bit set to
/// mark a thread over the lifetime of a thread.
/// Second, atomic groups are identified by
template <std::size_t tGroupCount>
class CaptureGroupManagerWithAtomic
    : public CaptureGroupManagerBase<tGroupCount, CaptureGroupSetWithAtomic<tGroupCount>> {

    using Base = CaptureGroupManagerBase<tGroupCount, CaptureGroupSetWithAtomic<tGroupCount>>;

public:
    auto markThreadWithAtomicGroupId(CaptureGroupSetReference reference, const AtomicGroupId atomicGroupId)
        -> CaptureGroupSetReference override {

        // Threads that start an atomic group are permanently marked by setting the atomic-group bit.
        // This bit isn't removed for the lifetime of this thread.
        auto [newReference, set] = Base::setForModification(reference);
        const auto groupMaskBit = static_cast<AtomicGroupMask>(1U) << atomicGroupId;
        set.setAtomicGroupMask(set.atomicGroupMask() | groupMaskBit);
        return newReference;
    }

    auto getAllReferencesForAtomicGroup(CaptureGroupSetReference reference, const AtomicGroupId atomicGroupId)
        -> CaptureGroupSetReferenceList override {

        const auto &set = Base::groupSet(reference);
        CaptureGroupSetReferenceList affectedGroupReferences;
        const auto groupMaskBit = static_cast<AtomicGroupMask>(1U) << atomicGroupId;
        // Collect all group references that have this atomic group mask bit set
        for (CaptureGroupSetReference ref = 0; ref < Base::_captureGroupSets.size(); ++ref) {
            const auto &otherSet = Base::_captureGroupSets[ref];
            if (otherSet.referenceCount() > 0 &&                         // only entries that are in use.
                set.atomicGroupRunId() == otherSet.atomicGroupRunId() && // test for the same run ID
                (otherSet.atomicGroupMask() & groupMaskBit) != 0) {      // test for the group bit.

                affectedGroupReferences.push_back(ref);
            }
        }
        return affectedGroupReferences;
    }
};

}
