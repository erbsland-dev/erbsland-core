// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CustomError.hpp"

#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void CustomError::operator()(impl::Rule &rule) {
    rule.setErrorMessage(std::move(_errorMessage));
}

}
