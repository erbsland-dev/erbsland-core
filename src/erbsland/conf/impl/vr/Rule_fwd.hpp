// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf::impl {

class Rule;
using RulePtr = std::shared_ptr<Rule>;
using RuleWeakPtr = std::weak_ptr<Rule>;

}
