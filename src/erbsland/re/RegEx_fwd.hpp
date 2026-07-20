// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re {

class RegEx;
/// A shared pointer to a regular expression.
using RegExPtr = std::shared_ptr<RegEx>;
/// A shared pointer to an immutable regular expression.
using ConstRegExPtr = std::shared_ptr<const RegEx>;

}
