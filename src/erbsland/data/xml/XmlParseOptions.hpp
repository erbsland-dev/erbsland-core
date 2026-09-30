// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../unit/ByteLength.hpp"
#include "../../unit/ItemCount.hpp"

namespace erbsland::data::xml {
/// Safety limits for parsing XML.
/// @tested{XmlDocumentTest}
class XmlParseOptions final {
public:
    static constexpr auto cDefaultMaximumInputLength = unit::ByteLength{16U * 1024U * 1024U}; ///< Input limit.
    static constexpr auto cDefaultMaximumNesting = unit::ItemCount{64U};                      ///< Element depth limit.
    static constexpr auto cDefaultMaximumNodeCount = unit::ItemCount{1'000'000U};             ///< Node limit.
    static constexpr auto cDefaultMaximumStringLength = unit::ByteLength{8U * 1024U * 1024U}; ///< Text byte limit.
public:
    /// Get the input byte limit.
    [[nodiscard]] constexpr auto maximumInputLength() const noexcept -> unit::ByteLength { return _maximumInputLength; }
    /// Set the input byte limit.
    constexpr auto setMaximumInputLength(unit::ByteLength value) noexcept -> XmlParseOptions & {
        _maximumInputLength = value;
        return *this;
    }
    /// Get the nesting limit.
    [[nodiscard]] constexpr auto maximumNesting() const noexcept -> unit::ItemCount { return _maximumNesting; }
    /// Set the nesting limit.
    constexpr auto setMaximumNesting(unit::ItemCount value) noexcept -> XmlParseOptions & {
        _maximumNesting = value;
        return *this;
    }
    /// Get the node count limit.
    [[nodiscard]] constexpr auto maximumNodeCount() const noexcept -> unit::ItemCount { return _maximumNodeCount; }
    /// Set the node count limit.
    constexpr auto setMaximumNodeCount(unit::ItemCount value) noexcept -> XmlParseOptions & {
        _maximumNodeCount = value;
        return *this;
    }
    /// Get the maximum byte length of one text or attribute value.
    [[nodiscard]] constexpr auto maximumStringLength() const noexcept -> unit::ByteLength {
        return _maximumStringLength;
    }
    /// Set the maximum byte length of one text or attribute value.
    constexpr auto setMaximumStringLength(unit::ByteLength value) noexcept -> XmlParseOptions & {
        _maximumStringLength = value;
        return *this;
    }

private:
    unit::ByteLength _maximumInputLength{cDefaultMaximumInputLength};   ///< Source bytes.
    unit::ItemCount _maximumNesting{cDefaultMaximumNesting};            ///< Element depth.
    unit::ItemCount _maximumNodeCount{cDefaultMaximumNodeCount};        ///< Nodes.
    unit::ByteLength _maximumStringLength{cDefaultMaximumStringLength}; ///< Text bytes.
};
}
