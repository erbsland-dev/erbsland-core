// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../../text/String.hpp"

#include <utility>

namespace erbsland::conf::vr::builder {

/// Sets the user-facing title of a rule.
struct Title : Attribute {
    explicit Title(text::String title) : _title{std::move(title)} {}
    void operator()(impl::Rule &rule) override;
    text::String _title;
};

}
