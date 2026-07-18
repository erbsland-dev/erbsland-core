// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U16StringLiteralStorage.hpp"
#include "U16StringSharedStorage.hpp"

#include <variant>

namespace erbsland::text::impl {

/// A view to shared or literal string data.
using U16StringStorage = std::variant<std::monostate, U16StringSharedStorage, U16StringLiteralStorage>;

}
