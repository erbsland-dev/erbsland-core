// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../JsonValue.hpp"

#include <cstdint>
#include <variant>

namespace erbsland::text::json::impl {

/// Shared storage for one JSON value.
/// @tested{JsonValueTest}
class JsonValueData final {
public:
    /// Variant storing all supported JSON representations.
    using Value = std::variant<std::monostate, bool, int64_t, double, String, JsonArray, JsonObject>;

public: // defaults
    JsonValueData() = default;

public: // construction
    /// Store a supported JSON representation.
    template <typename T>
    explicit JsonValueData(T value) : value{std::move(value)} {}

    Value value; ///< Stored JSON value.
};

}
