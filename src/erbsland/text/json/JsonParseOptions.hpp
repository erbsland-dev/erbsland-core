// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"
#include "../../unit/CpLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::text::json {

/// Safety limits for parsing JSON documents.
/// @tested{JsonParserTest}
class JsonParseOptions final {
public:
    static constexpr auto cDefaultMaximumInputLength = unit::ByteLength{16U * 1024U * 1024U}; ///< Input limit.
    static constexpr auto cDefaultMaximumNesting = unit::ItemCount{64U};                      ///< Nesting limit.
    static constexpr auto cDefaultMaximumValueCount = unit::ItemCount{1'000'000U};            ///< Value limit.
    static constexpr auto cDefaultMaximumStringLength = unit::CpLength{8U * 1024U * 1024U};   ///< String limit.

public:                                                                                       // accessors
    /// Get the maximum source length in bytes.
    [[nodiscard]] constexpr auto maximumInputLength() const noexcept -> unit::ByteLength { return _maximumInputLength; }
    /// Set the maximum source length in bytes.
    constexpr auto setMaximumInputLength(const unit::ByteLength value) noexcept -> JsonParseOptions & {
        _maximumInputLength = value;
        return *this;
    }
    /// Get the maximum number of open container levels.
    [[nodiscard]] constexpr auto maximumNesting() const noexcept -> unit::ItemCount { return _maximumNesting; }
    /// Set the maximum number of open container levels.
    constexpr auto setMaximumNesting(const unit::ItemCount value) noexcept -> JsonParseOptions & {
        _maximumNesting = value;
        return *this;
    }
    /// Get the maximum value count, including the root.
    [[nodiscard]] constexpr auto maximumValueCount() const noexcept -> unit::ItemCount { return _maximumValueCount; }
    /// Set the maximum value count, including the root.
    constexpr auto setMaximumValueCount(const unit::ItemCount value) noexcept -> JsonParseOptions & {
        _maximumValueCount = value;
        return *this;
    }
    /// Get the maximum decoded key or string length in Unicode code points.
    [[nodiscard]] constexpr auto maximumStringLength() const noexcept -> unit::CpLength { return _maximumStringLength; }
    /// Set the maximum decoded key or string length in Unicode code points.
    constexpr auto setMaximumStringLength(const unit::CpLength value) noexcept -> JsonParseOptions & {
        _maximumStringLength = value;
        return *this;
    }

private:
    unit::ByteLength _maximumInputLength{cDefaultMaximumInputLength}; ///< Maximum source bytes.
    unit::ItemCount _maximumNesting{cDefaultMaximumNesting};          ///< Maximum open containers.
    unit::ItemCount _maximumValueCount{cDefaultMaximumValueCount};    ///< Maximum total values.
    unit::CpLength _maximumStringLength{cDefaultMaximumStringLength}; ///< Maximum decoded string code points.
};

}
