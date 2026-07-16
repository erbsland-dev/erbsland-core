// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DataSection.hpp"

#include "../../../unit/LineIndex.hpp"
#include "../../../util/HashHelper.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace erbsland::re::impl {

/// The offset of a label.
using LabelOffset = uint32_t;

/// The target for a label.
struct LabelTarget {
    LabelTarget(const DataSection section, const LabelOffset offset) noexcept : section{section}, offset{offset} {}
    LabelTarget() = default;

    auto operator==(const LabelTarget &other) const noexcept -> bool = default;
    auto operator!=(const LabelTarget &other) const noexcept -> bool = default;

    DataSection section{DataSection::Program}; ///< The section for which the label is defined.
    LabelOffset offset{0};                     ///< The label offset (program counter/index).
};

/// A label target with source information.
struct LabelTargetWithSource : LabelTarget {
    LabelTargetWithSource(const DataSection section, const LabelOffset offset, const unit::LineIndex sourceLine) :
        LabelTarget{section, offset}, sourceLine{sourceLine} {}
    LabelTargetWithSource() = default;

    unit::LineIndex sourceLine; ///< The source line, where it was defined (for error reporting on duplicates).
};

}

namespace std {
template <>
struct hash<erbsland::re::impl::LabelTarget> {
    auto operator()(const erbsland::re::impl::LabelTarget &target) const noexcept -> std::size_t {
        return erbsland::util::createHash(target.section, target.offset);
    }
};
template <>
struct hash<erbsland::re::impl::LabelTargetWithSource> {
    auto operator()(const erbsland::re::impl::LabelTargetWithSource &target) const noexcept -> std::size_t {
        return erbsland::util::createHash(target.section, target.offset, target.sourceLine);
    }
};
}
