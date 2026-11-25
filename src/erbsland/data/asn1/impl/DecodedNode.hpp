// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "DecodedNode_fwd.hpp"

#include "../Asn1Node.hpp"

namespace erbsland::data::asn1::impl {

/// The validated fields transferred from the decoder into an immutable node.
/// @tested{Asn1NodeTest BerCodecTest}
class DecodedNode final {
    friend class Asn1Decoder;
    friend class asn1::Asn1Node;

    // defaults
    DecodedNode() = default;
    mem::ByteBlock encoded;               ///< Encoded identifier, length and content.
    unit::ByteIndex offset;               ///< Document-relative start.
    unit::ByteCount headerLength;         ///< Encoded identifier and length octets.
    Asn1Tag tag{Asn1UniversalType::None}; ///< Decoded identifier.
    bool constructed{};                   ///< Whether this value contains children.
    bool der{};                           ///< Whether strict DER rules validated this node.
    util::List<Asn1Node> children;        ///< Direct child values.
};

}
