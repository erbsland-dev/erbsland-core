// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Set the description of the implicit scalar entry of a value list.
/// @seedoc{/reference/conf/validation_rules}
/// @tested{VrEntryTypeTest}
class EntryDescription final : public Attribute {
public:
    /// Own the entry's description.
    /// @param description The description to copy or move into this attribute.
    explicit EntryDescription(text::String description) : _description{std::move(description)} {}

public: // implement Attribute
    void apply(RuleDefinition &rule) const override;

private:
    text::String _description; ///< The entry's description.
};

}
