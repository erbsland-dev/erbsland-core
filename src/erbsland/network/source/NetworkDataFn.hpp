// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"

#include <functional>

namespace erbsland::network {

/// A callback receiving an owned block of stream data.
using NetworkDataFn = std::function<void(mem::ByteBlock)>;

}
