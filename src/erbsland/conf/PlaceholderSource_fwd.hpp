// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::conf {

class PlaceholderSource;
/// Shared pointer for a placeholder source.
using PlaceholderSourcePtr = std::shared_ptr<PlaceholderSource>;

}
