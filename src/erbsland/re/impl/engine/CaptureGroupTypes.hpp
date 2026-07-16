// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>
#include <limits>
#include <set>
#include <vector>

namespace erbsland::re::impl {

/// A reference to a given group set.
using CaptureGroupSetReference = uint16_t;

/// A list of capture group set references.
using CaptureGroupSetReferenceList = std::vector<CaptureGroupSetReference>;

/// Special value to mark no capture group set.
constexpr auto cNoCaptureGroup = std::numeric_limits<CaptureGroupSetReference>::max();

/// The Atomic Group ID
/// This id references to the bit in the atomic group mask.
/// Therefore, it must be in the range 0...63.
using AtomicGroupId = uint16_t;

/// The atomic group mask.
/// 0 = no atomic group.
/// Each bit represents a unique atomic group.
/// For nested groups, the bits of the parent groups are inherited.
using AtomicGroupMask = uint64_t;

/// The Atomic Group Run ID
/// For overlapping parallel runs of a program, each run required a unique ID.
using AtomicGroupRunId = uint64_t;

/// Special value to mark no atomic group mask.
constexpr auto cNoAtomicGroupMask = 0U;

/// Special value to mark no atomic group ID.
constexpr auto cNoAtomicGroupId = std::numeric_limits<AtomicGroupId>::max();

}
