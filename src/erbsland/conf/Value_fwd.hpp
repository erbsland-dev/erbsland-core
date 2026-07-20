// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class Value;
using ValuePtr = std::shared_ptr<Value>;
using ConstValuePtr = std::shared_ptr<const Value>;

}
