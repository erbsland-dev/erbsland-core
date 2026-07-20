// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "IsOptional.hpp"

#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void IsOptional::operator()(impl::Rule &rule) {
    rule.setOptional(_isOptional);
}

}
