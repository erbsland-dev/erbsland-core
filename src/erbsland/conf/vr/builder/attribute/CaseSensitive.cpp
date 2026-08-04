// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "CaseSensitive.hpp"

#include "../../../impl/vr/Rule.hpp"

namespace erbsland::conf::vr::builder {

void CaseSensitive::operator()(Rule &rule) {
    rule.setCaseSensitivity(_caseSensitivity);
}

}
