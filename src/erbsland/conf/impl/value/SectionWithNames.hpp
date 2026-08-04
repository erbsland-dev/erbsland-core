// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Section.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the section with regular names.
class SectionWithNames final : public Section {
public:
    /// Creates a section with regular names.
    SectionWithNames() : Section{ValueType::SectionWithNames} {}
};

}
