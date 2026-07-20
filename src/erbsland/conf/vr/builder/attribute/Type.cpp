// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Type.hpp"

#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void Type::operator()(impl::Rule &rule) {
    rule.setType(_type);
}

}
