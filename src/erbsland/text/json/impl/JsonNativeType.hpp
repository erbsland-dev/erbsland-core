// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../JsonValue_fwd.hpp"

#include "../../String.hpp"

#include <concepts>
#include <cstdint>

namespace erbsland::text::json::impl {

/// A native type supported by the typed JSON accessors.
template <typename T>
concept JsonNativeType = std::same_as<T, bool> || std::same_as<T, int64_t> || std::same_as<T, double> ||
    std::same_as<T, String> || std::same_as<T, JsonArray> || std::same_as<T, JsonObject>;

}
