// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../error/InternalError.hpp"

#include "../../CaptureRange.hpp"

#include <array>
#include <limits>

namespace erbsland::re::impl {

/// A templatized version of the capture group set.
/// @tparam tGroupCount The maximum number of capture groups this set can handle.
template <std::size_t tGroupCount>
class CaptureGroupSet {
protected:
    using RefCountType = uint32_t;

public:
    explicit CaptureGroupSet() = default;

public: // reference counting.
    void increaseReferenceCount() {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            _referenceCount < std::numeric_limits<RefCountType>::max(),
            "CaptureGroupSet::increaseReferenceCount: overflow"_el);
        _referenceCount += 1;
    }
    void decreaseReferenceCount() {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(_referenceCount > 0, "CaptureGroupSet::decreaseReferenceCount: underflow"_el);
        _referenceCount -= 1;
    }
    [[nodiscard]] auto referenceCount() const noexcept -> RefCountType { return _referenceCount; }
    void setReferenceCount(const RefCountType referenceCount) noexcept { _referenceCount = referenceCount; }

public: // Access the ranges.
    [[nodiscard]] auto ranges() const noexcept -> const std::array<CaptureRange, tGroupCount> & { return _ranges; }
    void setBegin(const std::size_t captureGroup, const InputPosition inputPosition) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            captureGroup < tGroupCount, "CaptureGroupSet::setBegin: captureGroup out of bounds"_el);
        _ranges[captureGroup].setBegin(inputPosition);
    }
    void setEnd(const std::size_t captureGroup, const InputPosition inputPosition) {
        ERBSLAND_CORE_RE_REQUIRE_SAFETY(
            captureGroup < tGroupCount, "CaptureGroupSet::setEnd: captureGroup out of bounds"_el);
        _ranges[captureGroup].setEnd(inputPosition);
    }

protected:
    RefCountType _referenceCount{0};
    std::array<CaptureRange, tGroupCount> _ranges{};
};

}
