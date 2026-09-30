// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../BsonValue.hpp"

#include <variant>

namespace erbsland::data::bson::impl {
/// Shared storage for one BSON value.
/// @tested{BsonValueTest}
class BsonValueData final {
public:
    /// Stored variant of supported wire values.
    using Value = std::variant<
        std::monostate,
        bool,
        int32_t,
        int64_t,
        double,
        text::String,
        BsonBinary,
        time::DateTime,
        BsonArray,
        BsonDocument,
        BsonOpaqueValue>;
    /// Store one typed wire value.
    /// @tparam T The supported value type.
    /// @param value The value to store.
    template <typename T>
    explicit BsonValueData(T value) : value{std::move(value)} {}
    Value value; ///< Stored value.
};
}
