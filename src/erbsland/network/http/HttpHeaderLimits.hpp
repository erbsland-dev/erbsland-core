// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::network {

/// Resource limits captured by an ordered HTTP header collection.
/// @seedoc{/reference/network/http_values}
/// @tested{HttpHeadersTest}
class HttpHeaderLimits final {
public:
    /// Default maximum number of fields.
    static constexpr auto cDefaultMaximumFieldCount = unit::ItemCount{100U};
    /// Default maximum field-name byte length.
    static constexpr auto cDefaultMaximumNameLength = unit::ByteLength{256U};
    /// Default maximum field-value byte length.
    static constexpr auto cDefaultMaximumValueLength = unit::ByteLength{16U * 1024U};
    /// Default maximum aggregate serialized byte length.
    static constexpr auto cDefaultMaximumAggregateLength = unit::ByteLength{64U * 1024U};

public: // accessors
    /// Get the maximum number of fields.
    [[nodiscard]] constexpr auto maximumFieldCount() const noexcept -> unit::ItemCount { return _maximumFieldCount; }
    /// Set the maximum number of fields.
    constexpr auto setMaximumFieldCount(unit::ItemCount value) noexcept -> HttpHeaderLimits & {
        _maximumFieldCount = value;
        return *this;
    }
    /// Get the maximum field-name byte length.
    [[nodiscard]] constexpr auto maximumNameLength() const noexcept -> unit::ByteLength { return _maximumNameLength; }
    /// Set the maximum field-name byte length.
    constexpr auto setMaximumNameLength(unit::ByteLength value) noexcept -> HttpHeaderLimits & {
        _maximumNameLength = value;
        return *this;
    }
    /// Get the maximum field-value byte length.
    [[nodiscard]] constexpr auto maximumValueLength() const noexcept -> unit::ByteLength { return _maximumValueLength; }
    /// Set the maximum field-value byte length.
    constexpr auto setMaximumValueLength(unit::ByteLength value) noexcept -> HttpHeaderLimits & {
        _maximumValueLength = value;
        return *this;
    }
    /// Get the maximum aggregate serialized byte length.
    [[nodiscard]] constexpr auto maximumAggregateLength() const noexcept -> unit::ByteLength {
        return _maximumAggregateLength;
    }
    /// Set the maximum aggregate serialized byte length.
    constexpr auto setMaximumAggregateLength(unit::ByteLength value) noexcept -> HttpHeaderLimits & {
        _maximumAggregateLength = value;
        return *this;
    }

private:
    unit::ItemCount _maximumFieldCount{cDefaultMaximumFieldCount};            ///< Maximum field count.
    unit::ByteLength _maximumNameLength{cDefaultMaximumNameLength};           ///< Maximum field-name bytes.
    unit::ByteLength _maximumValueLength{cDefaultMaximumValueLength};         ///< Maximum field-value bytes.
    unit::ByteLength _maximumAggregateLength{cDefaultMaximumAggregateLength}; ///< Maximum serialized bytes.
};

}
