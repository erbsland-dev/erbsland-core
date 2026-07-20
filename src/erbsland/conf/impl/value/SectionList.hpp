// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ValueWithChildren.hpp"

namespace erbsland::conf::impl {

/// The value implementation for a section list.
class SectionList final : public ValueWithChildren {
public:
    [[nodiscard]] auto type() const noexcept -> ValueType override { return ValueType::SectionList; }
};

}
