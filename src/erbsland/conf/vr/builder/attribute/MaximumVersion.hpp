// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../Integer.hpp"

namespace erbsland::conf::vr::builder {

/// Restricts a rule to versions smaller than or equal to a maximum.
struct MaximumVersion : Attribute {
    explicit MaximumVersion(const Integer version, const bool isNegated = false) :
        _version{version}, _isNegated{isNegated} {}
    void operator()(impl::Rule &rule) override;
    Integer _version;
    bool _isNegated{false};
};

}
