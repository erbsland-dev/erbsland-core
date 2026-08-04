// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "LabelTarget.hpp"

#include <cstddef>
#include <functional>

namespace erbsland::re::impl {

/// A label target with source information.
struct LabelTargetWithSource : LabelTarget {
    /// Create a label target with its source line.
    /// @param section The target data section.
    /// @param offset The target label offset.
    /// @param sourceLine The source line that defined the label.
    LabelTargetWithSource(const DataSection section, const LabelOffset offset, const unit::LineIndex sourceLine) :
        LabelTarget{section, offset}, sourceLine{sourceLine} {}

    // defaults
    LabelTargetWithSource() = default;

    unit::LineIndex sourceLine; ///< The source line, where it was defined (for error reporting on duplicates).
};

}

template <>
struct std::hash<erbsland::re::impl::LabelTargetWithSource> {
    auto operator()(const erbsland::re::impl::LabelTargetWithSource &target) const noexcept -> std::size_t {
        return erbsland::util::createHash(target.section, target.offset, target.sourceLine);
    }
};
