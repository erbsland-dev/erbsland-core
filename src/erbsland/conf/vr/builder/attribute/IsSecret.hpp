// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

namespace erbsland::conf::vr::builder {

/// Marks a rule value as secret.
class IsSecret : public Attribute {
public:
    /// Set whether a rule value is secret.
    /// @param isSecret `true` to mark the value secret.
    explicit IsSecret(const bool isSecret = true) : _isSecret{isSecret} {}
    void operator()(Rule &rule) override;
    bool _isSecret{true};
};

}
