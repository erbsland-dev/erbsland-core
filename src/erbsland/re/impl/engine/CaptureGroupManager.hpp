// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "CaptureGroupManager_fwd.hpp"
#include "CaptureGroupNames.hpp"
#include "CaptureGroupTypes.hpp"

#include "../../CaptureGroup.hpp"
#include "../../InputPosition.hpp"

#include <ranges>

namespace erbsland::re::impl {

/// The capture group manager.
///
/// This manager handles unique capture group sets. The idea behind using this manager is that each thread just
/// needs to store a unique ID that represents a unique capture group state. Only when the capture state
/// changes, a new state and new unique ID must be introduced.
///
/// Using this manager is on average faster than copying the full capture group state on every program iteration.
///
class CaptureGroupManager {
public:
    // defaults
    virtual ~CaptureGroupManager() = default;

public: // initialize
    /// Initialize capture-group state for a program run.
    virtual void initialize() = 0;
    /// Reset capture-group state before the next find operation.
    virtual void resetForNextFind() = 0;

public:
    /// Create the zero-capture group entry.
    [[nodiscard]] virtual auto createGroupSet(InputPosition startPosition) noexcept -> CaptureGroupSetReference = 0;

    /// Create a capture group list from a given reference.
    /// @param reference The reference to use for the capture group list.
    /// @param names The capture group names to use for the capture group list.
    ///              This is excluding the zero-capture group.
    ///              The size of the names vector has to be (captureGroupCount - 1).
    [[nodiscard]] virtual auto createCaptureGroupList(
        CaptureGroupSetReference reference, const CaptureGroupNames &names) const -> CaptureGroupList = 0;

    /// Release a capture group set.
    virtual void release(CaptureGroupSetReference reference) = 0;

    /// Allocate a capture group set.
    virtual void allocate(CaptureGroupSetReference reference) = 0;

    /// Start a new capture.
    /// @param reference The reference to the capture group set.
    /// @param captureGroup The capture group index (0 = full match, 1+ = capture group).
    /// @param inputPosition The position in the input string.
    /// @return The new capture group set reference.
    virtual auto startCapture(
        CaptureGroupSetReference reference, CaptureGroupIndex captureGroup, InputPosition inputPosition)
        -> CaptureGroupSetReference = 0;

    /// Stop a capture.
    /// @param reference The reference to the capture group set.
    /// @param captureGroup The capture group index (0 = full match, 1+ = capture group).
    /// @param inputPosition The position in the input string.
    /// @return The new capture group set reference.
    virtual auto stopCapture(
        CaptureGroupSetReference reference, CaptureGroupIndex captureGroup, InputPosition inputPosition)
        -> CaptureGroupSetReference = 0;

    /// Mark a thread with an atomic group id.
    /// The mark is permanent and cannot be removed.
    /// @param reference The reference to the capture group set.
    /// @param atomicGroupId The atomic group id.
    /// @return The new capture group set reference.
    virtual auto markThreadWithAtomicGroupId(CaptureGroupSetReference reference, AtomicGroupId atomicGroupId)
        -> CaptureGroupSetReference = 0;

    /// Get a list of capture group references that are part of the same atomic group.
    /// @param reference The reference to the capture group set of the current thread.
    /// @param atomicGroupId The atomic group id.
    /// @return A list of affected group references with the potential to be terminated.
    virtual auto getAllReferencesForAtomicGroup(CaptureGroupSetReference reference, AtomicGroupId atomicGroupId)
        -> CaptureGroupSetReferenceList = 0;

public:
    /// Create a capture group manager.
    /// @param captureGroupCount The number of capture groups (including the zero group).
    /// @param hasAtomicGroups If the regular expression contains atomic groups.
    /// @return The capture group manager.
    [[nodiscard]] static auto create(std::size_t captureGroupCount, bool hasAtomicGroups) -> CaptureGroupManagerPtr;
};

}
