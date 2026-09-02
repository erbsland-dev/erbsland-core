// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BaseNFormat.hpp"

#include "../AnyString.hpp"

#include "../../mem/ByteBlock_fwd.hpp"
#include "../../unit/ByteLength.hpp"

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
    /// Decode data, returning no value for malformed text or a size-limit violation.
    [[nodiscard]] auto toData(unit::ByteLength maximum = unit::ByteLength::infinite()) const
        -> std::optional<mem::ByteBlock>;
    /// Decode data or throw a detailed error.
    /// @throws err::ParseError If the text is malformed.
    /// @throws err::OutOfRangeError If the decoded data exceeds `maximum`.
    [[nodiscard]] auto toDataOrThrow(unit::ByteLength maximum = unit::ByteLength::infinite()) const -> mem::ByteBlock;

private:
    AnyString _text;
    BaseNFormat _format;
};

}
