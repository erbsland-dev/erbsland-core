// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::data::cbor {
/// Limits and profile for parsing CBOR.
/// @tested{CborValueTest}
class CborParseOptions final {
public:
    static constexpr auto cDefaultMaximumInputLength = unit::ByteLength{16U * 1024U * 1024U}; ///< Input limit.
    static constexpr auto cDefaultMaximumNesting = unit::ItemCount{64U};           ///< Container depth limit.
    static constexpr auto cDefaultMaximumValueCount = unit::ItemCount{1'000'000U}; ///< Value count limit.
    static constexpr auto cDefaultMaximumStringLength = unit::ByteLength{8U * 1024U * 1024U}; ///< String byte limit.
public:
    /// Select strict DAG-CBOR parsing.
    [[nodiscard]] constexpr auto isDagCbor() const noexcept -> bool { return _dagCbor; }
    /// Select strict DAG-CBOR parsing.
    constexpr auto setDagCbor(bool value) noexcept -> CborParseOptions & {
        _dagCbor = value;
        return *this;
    }
    /// Get the input byte limit.
    [[nodiscard]] constexpr auto maximumInputLength() const noexcept -> unit::ByteLength { return _maximumInputLength; }
    /// Set the input byte limit.
    constexpr auto setMaximumInputLength(unit::ByteLength value) noexcept -> CborParseOptions & {
        _maximumInputLength = value;
        return *this;
    }
    /// Get the nesting limit.
    [[nodiscard]] constexpr auto maximumNesting() const noexcept -> unit::ItemCount { return _maximumNesting; }
    /// Set the nesting limit.
    constexpr auto setMaximumNesting(unit::ItemCount value) noexcept -> CborParseOptions & {
        _maximumNesting = value;
        return *this;
    }
    /// Get the value count limit.
    [[nodiscard]] constexpr auto maximumValueCount() const noexcept -> unit::ItemCount { return _maximumValueCount; }
    /// Set the value count limit.
    constexpr auto setMaximumValueCount(unit::ItemCount value) noexcept -> CborParseOptions & {
        _maximumValueCount = value;
        return *this;
    }
    /// Get the maximum byte length of one text string.
    [[nodiscard]] constexpr auto maximumStringLength() const noexcept -> unit::ByteLength {
        return _maximumStringLength;
    }
    /// Set the maximum byte length of one text string.
    constexpr auto setMaximumStringLength(unit::ByteLength value) noexcept -> CborParseOptions & {
        _maximumStringLength = value;
        return *this;
    }

private:
    bool _dagCbor{false};                                               ///< Strict DAG-CBOR profile.
    unit::ByteLength _maximumInputLength{cDefaultMaximumInputLength};   ///< Input bytes.
    unit::ItemCount _maximumNesting{cDefaultMaximumNesting};            ///< Container depth.
    unit::ItemCount _maximumValueCount{cDefaultMaximumValueCount};      ///< Total values.
    unit::ByteLength _maximumStringLength{cDefaultMaximumStringLength}; ///< Text bytes.
};
}
