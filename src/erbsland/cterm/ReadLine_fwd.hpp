// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <memory>

namespace erbsland::cterm {

class ReadLine;
/// A shared pointer to an interactive terminal line editor.
using ReadLinePtr = std::shared_ptr<ReadLine>;

}
