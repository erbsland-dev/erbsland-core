// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class AccessCheck;
/// Shared pointer for AccessCheck.
using AccessCheckPtr = std::shared_ptr<AccessCheck>;

}
