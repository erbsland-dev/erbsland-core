// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::cryptology {

/// ASN.1 universal tag numbers used by X.509 certificates.
enum class Asn1UniversalType : uint32_t { // NOLINT(*-enum-size)
    None = 0U,                            ///< The node does not use the universal tag class.
    Boolean = 1U,                         ///< A BOOLEAN value.
    Integer = 2U,                         ///< An INTEGER value.
    BitString = 3U,                       ///< A BIT STRING value.
    OctetString = 4U,                     ///< An OCTET STRING value.
    Null = 5U,                            ///< A NULL value.
    ObjectIdentifier = 6U,                ///< An OBJECT IDENTIFIER value.
    Utf8String = 12U,                     ///< A UTF8String value.
    Sequence = 16U,                       ///< A SEQUENCE value.
    Set = 17U,                            ///< A SET value.
    NumericString = 18U,                  ///< A NumericString value.
    PrintableString = 19U,                ///< A PrintableString value.
    TeletexString = 20U,                  ///< A TeletexString value.
    Ia5String = 22U,                      ///< An IA5String value.
    UtcTime = 23U,                        ///< A UTCTime value.
    GeneralizedTime = 24U,                ///< A GeneralizedTime value.
    UniversalString = 28U,                ///< A UniversalString value.
    BmpString = 30U,                      ///< A BMPString value.
};

}
