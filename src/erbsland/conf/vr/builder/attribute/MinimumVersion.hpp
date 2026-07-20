// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Attribute.hpp"

#include "../../../Integer.hpp"

namespace erbsland::conf::vr::builder {

/// Restricts a rule to versions greater than or equal to a minimum.
struct MinimumVersion : Attribute {
    explicit MinimumVersion(const Integer version, const bool isNegated = false) :
        _version{version}, _isNegated{isNegated} {}
    void operator()(impl::Rule &rule) override;
    Integer _version;
    bool _isNegated{false};
};

}
