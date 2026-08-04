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
    /// Create a label target in a data section.
    /// @param section The target data section.
    /// @param offset The target label offset.
    LabelTarget(const DataSection section, const LabelOffset offset) noexcept : section{section}, offset{offset} {}

    // defaults
    LabelTarget() = default;
    auto operator==(const LabelTarget &other) const noexcept -> bool = default;
    auto operator!=(const LabelTarget &other) const noexcept -> bool = default;

    DataSection section{DataSection::Program}; ///< The section for which the label is defined.
    LabelOffset offset{0};                     ///< The label offset (program counter/index).
};

}

template <>
struct std::hash<erbsland::re::impl::LabelTarget> {
    auto operator()(const erbsland::re::impl::LabelTarget &target) const noexcept -> std::size_t {
        return erbsland::util::createHash(target.section, target.offset);
    }
};
