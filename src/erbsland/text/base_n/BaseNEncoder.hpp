// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "BaseNFormat.hpp"

#include "../AnyString_fwd.hpp"
#include "../String_fwd.hpp"
#include "../u16/U16String_fwd.hpp"
#include "../u32/U32String_fwd.hpp"
#include "../u8/U8String_fwd.hpp"

#include "../../mem/ByteBlock.hpp"

namespace erbsland::text::base_n {

/// Encode binary data using a validated Base-N format.
/// @seedoc{/reference/text/base_n}
/// @tested{BaseNCodecTest}
class BaseNEncoder final {
public:
    /// Create an encoder for data and a format.
    explicit BaseNEncoder(mem::ByteBlock data, BaseNFormat format = BaseNFormat::defaultFormat());

public: // accessors
    /// Get the encoding format.
    [[nodiscard]] auto format() const noexcept -> const BaseNFormat & { return _format; }

public: // conversion
    /// Encode into the primary UTF-8 string type.
    [[nodiscard]] auto toString() const -> String;
    /// Encode into UTF-8.
    [[nodiscard]] auto toU8String() const -> U8String;
    /// Encode into UTF-16.
    [[nodiscard]] auto toU16String() const -> U16String;
    /// Encode into UTF-32.
    [[nodiscard]] auto toU32String() const -> U32String;

private:
    /// Build the encoded text using the requested string kind.
    [[nodiscard]] auto build(StringKind kind) const -> AnyString;

private:
    mem::ByteBlock _data;
    BaseNFormat _format;
};

}
