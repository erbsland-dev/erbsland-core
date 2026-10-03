// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BaseNFormat.hpp"

#include "../AnyString.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../unit/ByteCount.hpp"

#include <optional>

namespace erbsland::text::base_n {

/// Decode Base-N text into binary data.
/// @seedoc{/reference/text/encoding}
/// @tested{BaseNCodecTest}
class BaseNDecoder final {
public:
    /// Create a decoder for text and a format.
    explicit BaseNDecoder(AnyString text, BaseNFormat format = BaseNFormat::defaultFormat());

public: // accessors
    /// Get the decoding format.
    [[nodiscard]] auto format() const noexcept -> const BaseNFormat & { return _format; }

public: // conversion
    /// Decode data, preserving sensitive input storage and returning no value for invalid input.
    [[nodiscard]] auto toData(unit::ByteCount maximum = unit::ByteCount::infinite()) const
        -> std::optional<mem::ByteBlock>;
    /// Decode data, preserving sensitive input storage, or throw a detailed error.
    /// @param maximum The decoded byte limit, or infinite for no limit. Exceeding this limit is a parse failure.
    /// @throws err::ParseError If the input is malformed or the decoded data exceeds `maximum`.
    [[nodiscard]] auto toDataOrThrow(unit::ByteCount maximum = unit::ByteCount::infinite()) const -> mem::ByteBlock;

private:
    AnyString _text;
    BaseNFormat _format;
};

}
