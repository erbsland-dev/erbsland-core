// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

namespace erbsland::conf::vr::builder {

/// Marks a rule value as secret.
struct IsSecret : Attribute {
    explicit IsSecret(const bool isSecret = true) : _isSecret{isSecret} {}
    void operator()(impl::Rule &rule) override;
    bool _isSecret{true};
};

}
