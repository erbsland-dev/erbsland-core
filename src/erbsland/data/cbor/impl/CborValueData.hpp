// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../CborValue.hpp"

#include <variant>

namespace erbsland::data::cbor::impl {
/// Shared storage for one CBOR value.
/// @tested{CborValueTest}
class CborValueData final {
public:
    /// Stored variant of supported wire values.
    using Value = std::variant<
        std::monostate,
        bool,
        int64_t,
        uint64_t,
        double,
        text::String,
        mem::ByteBlock,
        CborArray,
        CborMap,
        time::Timestamp,
        CborLink>;
    /// Store one typed wire value.
    /// @tparam T The supported value type.
    /// @param value The value to store.
    template <typename T>
    explicit CborValueData(T value) : value{std::move(value)} {}
    Value value; ///< Stored value.
};
}
