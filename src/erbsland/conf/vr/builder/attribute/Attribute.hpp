// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../../../text/String.hpp"

namespace erbsland::conf::impl {
class Rule;
}

namespace erbsland::conf::vr::builder {

/// Base interface for all rule builder attributes.
struct Attribute {
    virtual ~Attribute() = default;
    virtual void operator()(impl::Rule &rule) = 0;

protected:
    [[noreturn]] static void throwValidationError(text::String message);
};

}
