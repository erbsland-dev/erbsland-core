// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../String.hpp"
#include "../TextNodeData.hpp"

#include "../../unit/ColumnCount.hpp"
#include "../../unit/ColumnIndex.hpp"
#include "../../unit/ColumnRange.hpp"

namespace erbsland::text::impl {

/// Logical source position data attached to a code-line marker node.
class CodeLineMarkerData final : public TextNodeData {
public:
    /// Create a point marker at column zero.
    CodeLineMarkerData() = default;
    /// Create a marker range.
    explicit CodeLineMarkerData(unit::ColumnRange range) noexcept : _range{range} {}
    /// Create a marker from a logical source column and length.
    CodeLineMarkerData(unit::ColumnIndex column, unit::ColumnCount length = {}) noexcept : _range{column, length} {}

public:
    /// Access the logical source range.
    [[nodiscard]] auto range() const noexcept -> unit::ColumnRange { return _range; }
    [[nodiscard]] auto toString() const -> String override;

private:
    unit::ColumnRange _range; ///< The logical source range.
    mutable String _text;     ///< The cached diagnostic representation.
};

}
