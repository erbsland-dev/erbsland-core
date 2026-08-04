// Copyright (c) 2024-2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Section.hpp"

namespace erbsland::conf::impl {

/// The value implementation for the section with text names.
class SectionWithTexts final : public Section {
public:
    /// Creates a section with text names.
    SectionWithTexts() : Section{ValueType::SectionWithTexts} {}
};

}
