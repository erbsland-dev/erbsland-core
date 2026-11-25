// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Asn1TagClass.hpp"
#include "Asn1UniversalType.hpp"

#include <compare>

namespace erbsland::data::asn1 {

/// The class and number of an ASN.1 identifier, ordered according to X.680.
/// @tested{Asn1NodeTest BerCodecTest}
struct Asn1Tag final {
    /// Create a universal tag.
    /// @param type The universal type.
    constexpr Asn1Tag(Asn1UniversalType type) noexcept :
        tagClass{Asn1TagClass::Universal}, number{static_cast<uint32_t>(type)} {}
    /// Create a tag in the given class.
    /// @param tagClass The ASN.1 class.
    /// @param number The numeric tag.
    constexpr Asn1Tag(Asn1TagClass tagClass, uint32_t number) noexcept : tagClass{tagClass}, number{number} {}
    auto operator<=>(const Asn1Tag &) const noexcept = default;
    Asn1TagClass tagClass; ///< Identifier class.
    uint32_t number;       ///< Tag number, including high-tag-number form.
};

}
