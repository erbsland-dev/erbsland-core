// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager.hpp"
#include "CaptureGroupSet.hpp"

#include "../error/InternalError.hpp"

#include "../../CaptureRange.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <tuple>
#include <vector>

namespace erbsland::re::impl {

/// The template subclass of the capture group manager.
/// @tparam tGroupCount The maximum number of capture groups this manager can handle.
/// @tparam GroupSet The type of capture group set to use.
template <std::size_t tGroupCount, typename GroupSet = CaptureGroupSet<tGroupCount>>
class CaptureGroupManagerBase : public CaptureGroupManager {
public:
    CaptureGroupManagerBase() = default;

public: // initialize
    void initialize() override { reserveInitialCapacity(); }

    void resetForNextFind() override {
        _captureGroupSets.clear();
        _firstFreeSlot = 0;
    }

public:
    [[nodiscard]] auto createGroupSet(const InputPosition startPosition) noexcept -> CaptureGroupSetReference override {
        auto set = GroupSet{};
        set.setBegin(0, startPosition);
        return addSet(std::move(set));
    }

    [[nodiscard]] auto createCaptureGroupList(
        const CaptureGroupSetReference reference, const CaptureGroupNames &names) const -> CaptureGroupList override {

        const std::size_t actualGroupCount = names.size() + 1;
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            actualGroupCount <= tGroupCount, "createCaptureGroupList: actualGroupCount exceeds manager capacity"_el);

        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            reference < _captureGroupSets.size(), "createCaptureGroupList: reference out of bounds"_el);
        const auto &set = _captureGroupSets[reference];
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(set.referenceCount() > 0U, "createCaptureGroupList: referenceCount is zero"_el);
        CaptureGroupList captureGroups;
        captureGroups.reserve(actualGroupCount);
        for (std::size_t i = 0U; i < actualGroupCount; ++i) {
            const auto index = static_cast<CaptureGroupIndex>(i);
            if (i == 0) {
                captureGroups.emplace_back(index, set.ranges()[i], text::StringView{});
            } else {
                captureGroups.emplace_back(index, set.ranges()[i], names[i - 1]);
            }
        }
        return captureGroups;
    }

    void release(const CaptureGroupSetReference reference) override {
        if (reference == cNoCaptureGroup) {
            return;
        }
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(reference < _captureGroupSets.size(), "release: reference out of bounds"_el);
        auto &set = _captureGroupSets[reference];
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            set.referenceCount() > 0, "release: referenceCount already zero (double release)"_el);
        set.decreaseReferenceCount();
        if (set.referenceCount() == 0 && reference < _firstFreeSlot) {
            _firstFreeSlot = reference;
        }
    }

    void allocate(const CaptureGroupSetReference reference) override {
        if (reference == cNoCaptureGroup) {
            return;
        }
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(reference < _captureGroupSets.size(), "allocate: reference out of bounds"_el);
        _captureGroupSets[reference].increaseReferenceCount();
    }

    auto startCapture(
        const CaptureGroupSetReference reference, CaptureGroupIndex captureGroup, const InputPosition inputPosition)
        -> CaptureGroupSetReference override {

        auto [newReference, set] = setForModification(reference);
        set.setBegin(captureGroup, inputPosition);
        return newReference;
    }

    auto stopCapture(
        const CaptureGroupSetReference reference, CaptureGroupIndex captureGroup, const InputPosition inputPosition)
        -> CaptureGroupSetReference override {

        auto [newReference, set] = setForModification(reference);
        set.setEnd(captureGroup, inputPosition);
        return newReference;
    }

    auto markThreadWithAtomicGroupId(CaptureGroupSetReference /*reference*/, AtomicGroupId /*atomicGroupId*/)
        -> CaptureGroupSetReference override {

        throwInternalError("markThreadWithAtomicGroupId() called on group manager with no atomic groups"_el);
    }

    auto getAllReferencesForAtomicGroup(CaptureGroupSetReference /*reference*/, AtomicGroupId /*atomicGroupId*/)
        -> CaptureGroupSetReferenceList override {

        throwInternalError("CaptureGroupSetReferenceList() called on group manager with no atomic groups"_el);
    }

    [[nodiscard]] auto groupSet(const CaptureGroupSetReference reference) const noexcept -> const GroupSet & {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(reference < _captureGroupSets.size(), "groupSet: reference out of bounds"_el);
        return _captureGroupSets[reference];
    }

protected:
    [[nodiscard]] auto setForModification(CaptureGroupSetReference reference)
        -> std::tuple<CaptureGroupSetReference, GroupSet &> {

        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            reference < _captureGroupSets.size(), "setForModification: reference out of bounds"_el);
        auto &set = _captureGroupSets[reference];
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(set.referenceCount() > 0, "setForModification: referenceCount is zero"_el);
        if (set.referenceCount() == 1) {
            return {reference, set};
        }
        set.decreaseReferenceCount();
        const auto newReference = addSet(set);
        return {newReference, _captureGroupSets[newReference]};
    }

    [[nodiscard]] auto addSet(GroupSet captureGroupSet) -> CaptureGroupSetReference {
        captureGroupSet.setReferenceCount(1);

        const auto emptySlot = findFirstFreeSlot();
        if (emptySlot != cNoSlot) {
            _captureGroupSets[emptySlot] = std::move(captureGroupSet);
            _firstFreeSlot = emptySlot + 1;
            return emptySlot;
        }

        _firstFreeSlot = _captureGroupSets.size();
        const auto reference = static_cast<CaptureGroupSetReference>(_captureGroupSets.size());
        _captureGroupSets.emplace_back(std::move(captureGroupSet));
        return reference;
    }

    /// Directly "delete" a set (used for atomic group prune).
    void deleteSet(const CaptureGroupSetReference reference) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(reference < _captureGroupSets.size(), "deleteSet: reference out of bounds"_el);
        _captureGroupSets[reference].setReferenceCount(0);
        if (reference < _firstFreeSlot) {
            _firstFreeSlot = reference;
        }
    }

private:
    static constexpr auto cNoSlot = std::numeric_limits<CaptureGroupSetReference>::max();

    void reserveInitialCapacity() {
        // Reserve by bytes instead of tying the initial capacity to the number of capture groups.
        // This keeps the initial allocation within a predictable size range and automatically
        // accounts for the different `sizeof(GroupSet)` between atomic and non-atomic managers.
        constexpr std::size_t cTargetReserveBytes = 16U * 1024U;
        constexpr std::size_t cMinimumReserveCount = 16U;
        constexpr std::size_t cMaximumReserveCount = 256U;
        const auto reserveCount = std::clamp(
            cTargetReserveBytes / std::max<std::size_t>(1U, sizeof(GroupSet)),
            cMinimumReserveCount,
            cMaximumReserveCount);
        _captureGroupSets.reserve(reserveCount);
    }

    [[nodiscard]] auto findFirstFreeSlot() const -> CaptureGroupSetReference {
        for (std::size_t i = _firstFreeSlot; i < _captureGroupSets.size(); ++i) {
            if (_captureGroupSets[i].referenceCount() == 0) {
                return static_cast<CaptureGroupSetReference>(i);
            }
        }
        return cNoSlot;
    }

protected:
    std::vector<GroupSet> _captureGroupSets{};
    std::size_t _firstFreeSlot{0};
};

}
