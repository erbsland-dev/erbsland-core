// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::cterm {

class Font;
/// Shared pointer to a terminal font.
using FontPtr = std::shared_ptr<Font>;

}
