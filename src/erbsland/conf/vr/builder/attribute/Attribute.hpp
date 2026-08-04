// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"
#include "../../../impl/vr/KeyConstraint_fwd.hpp"
#include "../../../impl/vr/Rule_fwd.hpp"
#include "../../../impl/vr/VersionMask_fwd.hpp"

namespace erbsland::conf::vr::builder {

/// Base interface for all rule builder attributes.
class Attribute {
public:
    using Rule = impl::Rule;

    // defaults
    virtual ~Attribute() = default;
    /// Apply this attribute to a rule.
    virtual void operator()(Rule &rule) = 0;

protected:
    /// Throw an error that explains invalid attribute data.
    [[noreturn]] static void throwValidationError(text::String message);
};

}
