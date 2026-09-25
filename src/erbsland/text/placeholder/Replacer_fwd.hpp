// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::text::placeholder {

class Replacer;
/// Shared pointer to a configured placeholder replacer.
using ReplacerPtr = std::shared_ptr<Replacer>;

}
