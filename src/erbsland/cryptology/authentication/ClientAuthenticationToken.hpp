// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AuthenticationTokenPair_fwd.hpp"
#include "ClientAuthenticationToken_fwd.hpp"
#include "ServerAuthenticationToken_fwd.hpp"

#include "../keys/SigningPrivateKey.hpp"
#include "../SignedByteBlock_fwd.hpp"

#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// A move-only client credential containing a protected Ed25519 private key.
/// Its text representation is sensitive and must not be transmitted during authentication.
/// @seedoc{/reference/cryptology/authentication_tokens}
/// @tested{AuthenticationTokenTest}
class ClientAuthenticationToken final {
    friend class AuthenticationTokenPair;
    friend class ServerAuthenticationToken;

public:
    /// Create an empty placeholder.
    ClientAuthenticationToken() noexcept = default;

    // defaults/deletions
    ~ClientAuthenticationToken() = default;
    ClientAuthenticationToken(const ClientAuthenticationToken &) = delete;
    ClientAuthenticationToken(ClientAuthenticationToken &&) noexcept = default;
    auto operator=(const ClientAuthenticationToken &) -> ClientAuthenticationToken & = delete;
    auto operator=(ClientAuthenticationToken &&) noexcept -> ClientAuthenticationToken & = default;

public: // main operations
    /// Sign a complete challenge using this client's private key.
    /// @param challenge The exact challenge bytes supplied by the server.
    /// @return A purpose-separated signed response.
    /// @throws err::ParseError If the challenge is malformed or addresses another identifier.
    /// @throws err::LogicError If this token is empty.
    [[nodiscard]] auto createResponse(mem::ConstByteSpan challenge) const -> SignedByteBlock;

public: // tests and accessors
    /// Test whether this credential has no private key.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _key.isEmpty(); }
    /// Get the public identifier sent to the server for credential lookup.
    [[nodiscard]] auto id() const noexcept -> const text::String & { return _id; }

public: // conversion
    /// Return the sensitive canonical `elctk-<id>-c-<data>` text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse client credential text, returning an empty placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &token) noexcept -> ClientAuthenticationToken;
    /// Parse client credential text.
    /// @throws err::ParseError If the format, role, key, or encoding is invalid.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &token) -> ClientAuthenticationToken;

private:
    /// Store a validated identifier and protected signing key.
    ClientAuthenticationToken(text::String id, SigningPrivateKey key) noexcept;

private:
    text::String _id;       ///< Public lookup identifier.
    SigningPrivateKey _key; ///< Protected client private key.
};

}
