// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IsSecret.hpp"

#include "../../../../text/StringFormat.hpp"
#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

using namespace text::literals;

void IsSecret::operator()(Rule &rule) {
    if (_isSecret && !rule.type().isScalar()) {
        throwValidationError(
            text::StringFormat{"The 'is_secret' marker can only be used for scalar value types. Found {} type"_el}
                .build(rule.type().toText()));
    }
    rule.setSecret(_isSecret);
}

}
