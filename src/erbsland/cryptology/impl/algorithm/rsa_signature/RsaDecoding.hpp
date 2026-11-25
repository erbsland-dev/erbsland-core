// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "Number.hpp"

#include "../../../../data/asn1/Asn1Node_fwd.hpp"
#include "../../../../text/String_fwd.hpp"

#include <cstddef>

namespace erbsland::cryptology::impl::rsa_signer::decoder {

/// Reject an invalid private-key encoding or inconsistent key components.
/// @param reason The diagnostic describing the invalid input.
/// @throws err::ParseError Always, with the given diagnostic.
/// @tested{SigningPrivateKeyTest}
[[noreturn]] void fail(const text::String &reason);
/// Decode a positive DER integer into bounded RSA arithmetic storage.
/// @param node The DER integer node.
/// @param name The component name used in diagnostics.
/// @param count The destination word count.
/// @param sensitive Whether the temporary integer bytes require secure erasure.
/// @return The decoded integer at the requested width.
/// @throws err::ParseError If the node is not a valid positive DER integer.
/// @tested{SigningPrivateKeyTest}
[[nodiscard]] auto parseInteger(
    const data::asn1::Asn1Node &node, const text::String &name, std::size_t count, bool sensitive)
    -> rsa_signature::Number;

}
