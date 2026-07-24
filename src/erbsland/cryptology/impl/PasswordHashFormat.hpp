// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PasswordHashData_fwd.hpp"

#include "../PasswordHashPolicy.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"

#include <optional>
#include <span>

namespace erbsland::cryptology::impl {

/// Encode bytes as unpadded canonical Base64url.
/// @param bytes The bytes to encode.
/// @return The canonical Base64url text.
/// @tested{PasswordHasherTest}
[[nodiscard]] auto encodePasswordHashBytes(mem::ConstByteSpan bytes) -> text::String;
/// Decode canonical unpadded Base64url bytes.
/// @param text The encoded text.
/// @param expectedLength The required decoded length.
/// @return The decoded byte block.
/// @throws err::ParseError If the text is malformed, noncanonical, or has the wrong decoded length.
/// @tested{PasswordHasherTest}
[[nodiscard]] auto decodePasswordHashBytes(const text::String &text, unit::ByteLength expectedLength) -> mem::ByteBlock;
/// Build the canonical password-hash header through the salt field.
/// @param policy The algorithm and cost policy.
/// @param keyed Whether application-key protection is enabled.
/// @param keyIdentifier The optional public key-rotation identifier.
/// @param salt The public salt.
/// @return The canonical header.
/// @tested{PasswordHasherTest}
[[nodiscard]] auto buildPasswordHashHeader(
    const PasswordHashPolicy &policy,
    bool keyed,
    const std::optional<text::String> &keyIdentifier,
    mem::ConstByteSpan salt) -> text::String;
/// Append a verifier to a canonical password-hash header.
/// @param header The canonical header through the salt field.
/// @param verifier The stored verifier bytes.
/// @return The complete canonical record.
/// @tested{PasswordHasherTest}
[[nodiscard]] auto buildPasswordHashRecord(const text::String &header, mem::ConstByteSpan verifier) -> text::String;
/// Parse and validate a canonical password-hash record.
/// @param text The storage record.
/// @return The immutable parsed record data.
/// @throws err::ParseError If the record is malformed, noncanonical, unsupported, or exceeds resource limits.
/// @tested{PasswordHasherTest}
[[nodiscard]] auto parsePasswordHash(const text::String &text) -> PasswordHashDataPtr;

}
