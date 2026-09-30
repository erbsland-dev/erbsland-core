// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "impl/BsonCodec_fwd.hpp"

#include "../../mem/ByteBlock.hpp"

#include <cstdint>

namespace erbsland::data::bson {
/// Known BSON wire value preserved without executing or interpreting it.
/// @tested{BsonValueTest}
class BsonOpaqueValue final {
    friend class impl::BsonCodec;

private:
    /// Construct a preserved wire value or DOM node.
    BsonOpaqueValue(uint8_t typeCode, mem::ByteBlock payload) : _typeCode{typeCode}, _payload{std::move(payload)} {}

public:
    // defaults
    BsonOpaqueValue(const BsonOpaqueValue &) = default;
    BsonOpaqueValue(BsonOpaqueValue &&) = default;
    auto operator=(const BsonOpaqueValue &) -> BsonOpaqueValue & = default;
    auto operator=(BsonOpaqueValue &&) -> BsonOpaqueValue & = default;
    /// Get the BSON type code.
    [[nodiscard]] auto typeCode() const noexcept -> uint8_t { return _typeCode; }
    /// Get the encoded value payload, excluding type and key.
    [[nodiscard]] auto payload() const noexcept -> mem::ByteBlock { return _payload; }

private:
    uint8_t _typeCode;       ///< BSON wire type.
    mem::ByteBlock _payload; ///< Framed wire payload.
};
}
