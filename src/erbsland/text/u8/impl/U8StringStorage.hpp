// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U8StringLiteralStorage.hpp"
#include "U8StringSharedStorage.hpp"

#include <variant>

namespace erbsland::text::impl {

/// A view to shared or literal string data.
using U8StringStorage = std::variant<std::monostate, U8StringSharedStorage, U8StringLiteralStorage>;

}
