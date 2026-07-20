// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

namespace erbsland::conf::vr::builder {

/// Marks a rule as optional or required.
struct IsOptional : Attribute {
    explicit IsOptional(const bool isOptional = true) : _isOptional{isOptional} {}
    void operator()(impl::Rule &rule) override;
    bool _isOptional{true};
};

}
