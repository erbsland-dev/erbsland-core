// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "U32StringLiteralStorage.hpp"
#include "U32StringSharedStorage.hpp"

#include <variant>

namespace erbsland::text::impl {

/// A view to shared or literal string data.
using U32StringViewStorage = std::variant<std::monostate, U32StringSharedStorage, U32StringLiteralStorage>;

}
