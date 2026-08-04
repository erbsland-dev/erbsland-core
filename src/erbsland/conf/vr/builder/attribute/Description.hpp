// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Sets the descriptive text of a rule.
class Description : public Attribute {
public:
    /// Set descriptive text for a rule.
    /// @param description The text to move into the attribute.
    explicit Description(text::String description) : _description{std::move(description)} {}
    void operator()(Rule &rule) override;
    text::String _description;
};

}
