// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CustomError.hpp"

namespace erbsland::conf::vr::builder {

void CustomError::apply(RuleDefinition &rule) const {
    rule.setErrorMessage(_errorMessage);
}

}
