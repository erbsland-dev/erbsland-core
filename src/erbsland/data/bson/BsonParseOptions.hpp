// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteCount.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::data::bson {

/// Safety limits for parsing BSON.
/// @tested{BsonValueTest}
class BsonParseOptions final {
public:
    static constexpr auto cDefaultMaximumInputLength = unit::ByteCount{16U * 1024U * 1024U}; ///< Document byte limit.
    static constexpr auto cDefaultMaximumNesting = unit::ItemCount{64U};                     ///< Container depth limit.
    static constexpr auto cDefaultMaximumValueCount = unit::ItemCount{1'000'000U};           ///< Value count limit.
    static constexpr auto cDefaultMaximumStringLength = unit::ByteCount{8U * 1024U * 1024U}; ///< String byte limit.
public:
    /// Get the input byte limit.
    [[nodiscard]] constexpr auto maximumInputLength() const noexcept -> unit::ByteCount { return _maximumInputLength; }
    /// Set the input byte limit.
    constexpr auto setMaximumInputLength(unit::ByteCount value) noexcept -> BsonParseOptions & {
        _maximumInputLength = value;
        return *this;
    }
    /// Get the nesting limit.
    [[nodiscard]] constexpr auto maximumNesting() const noexcept -> unit::ItemCount { return _maximumNesting; }
    /// Set the nesting limit.
    constexpr auto setMaximumNesting(unit::ItemCount value) noexcept -> BsonParseOptions & {
        _maximumNesting = value;
        return *this;
    }
    /// Get the value count limit.
    [[nodiscard]] constexpr auto maximumValueCount() const noexcept -> unit::ItemCount { return _maximumValueCount; }
    /// Set the value count limit.
    constexpr auto setMaximumValueCount(unit::ItemCount value) noexcept -> BsonParseOptions & {
        _maximumValueCount = value;
        return *this;
    }
    /// Get the maximum byte length of one string or key.
    [[nodiscard]] constexpr auto maximumStringLength() const noexcept -> unit::ByteCount {
        return _maximumStringLength;
    }
    /// Set the maximum byte length of one string or key.
    constexpr auto setMaximumStringLength(unit::ByteCount value) noexcept -> BsonParseOptions & {
        _maximumStringLength = value;
        return *this;
    }

private:
    unit::ByteCount _maximumInputLength{cDefaultMaximumInputLength};   ///< Input bytes.
    unit::ItemCount _maximumNesting{cDefaultMaximumNesting};           ///< Container depth.
    unit::ItemCount _maximumValueCount{cDefaultMaximumValueCount};     ///< Values.
    unit::ByteCount _maximumStringLength{cDefaultMaximumStringLength}; ///< Text bytes.
};

}
