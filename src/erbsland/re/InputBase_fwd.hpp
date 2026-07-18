// Copyright (c) 2025-2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::re {

class InputBase;
/// A shared pointer to an input instance.
using InputBasePtr = std::shared_ptr<InputBase>;

}
