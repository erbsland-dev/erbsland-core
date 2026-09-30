// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology::impl::authentication_codec {

/// Test the canonical `[_a-z0-9]{1,64}` identifier grammar.
[[nodiscard]] auto isValidIdentifier(const text::String &identifier) noexcept -> bool;
/// Encode bytes as canonical, unpadded Base64url text.
[[nodiscard]] auto encodeBase64Url(mem::ConstByteSpan data, bool sensitive = false) -> text::String;
/// Decode exactly `expectedLength` bytes of canonical, unpadded Base64url text.
[[nodiscard]] auto decodeBase64Url(const text::String &text, std::size_t expectedLength, bool sensitive = false)
    -> mem::ByteBlock;
/// Build one canonical text credential.
[[nodiscard]] auto encodeToken(const text::String &identifier, char role, mem::ConstByteSpan keyData) -> text::String;
/// Parse one exact credential and return its 32-byte key data.
[[nodiscard]] auto decodeToken(const text::String &text, char expectedRole, text::String &identifier) -> mem::ByteBlock;
/// Encode an authentication challenge with an exact 32-byte nonce.
[[nodiscard]] auto encodeChallenge(const text::String &identifier, mem::ConstByteSpan nonce) -> mem::ByteBlock;
/// Validate a complete authentication challenge and return its identifier.
[[nodiscard]] auto decodeChallengeIdentifier(mem::ConstByteSpan challenge) -> text::String;

}
