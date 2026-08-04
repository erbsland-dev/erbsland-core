// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Sets the user-facing title of a rule.
class Title : public Attribute {
public:
    /// Set the user-facing rule title.
    /// @param title The title to move into the attribute.
    explicit Title(text::String title) : _title{std::move(title)} {}
    void operator()(Rule &rule) override;
    text::String _title;
};

}
