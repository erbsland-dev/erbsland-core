// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::text::placeholder {

class Source;
/// Shared pointer for a placeholder source.
using SourcePtr = std::shared_ptr<Source>;

}
