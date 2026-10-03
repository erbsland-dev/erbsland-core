// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Set the title of the implicit scalar entry of a value list.
/// @seedoc{/reference/conf/validation_rules}
/// @tested{VrEntryTypeTest}
class EntryTitle final : public Attribute {
public:
    /// Own the entry's title.
    /// @param title The title to copy or move into this attribute.
    explicit EntryTitle(text::String title) : _title{std::move(title)} {}

public: // implement Attribute
    void apply(RuleDefinition &rule) const override;

private:
    text::String _title; ///< The entry's title.
};

}
