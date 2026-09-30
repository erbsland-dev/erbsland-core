// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "AuthenticationTokenPair_fwd.hpp"
#include "ClientAuthenticationToken_fwd.hpp"
#include "PendingAuthenticationChallenge_fwd.hpp"
#include "ServerAuthenticationToken_fwd.hpp"

#include "../keys/PublicKey.hpp"

#include "../../text/String.hpp"
#include "../../time/TimeAmounts.hpp"
#include "../../time/TimeDelta.hpp"

namespace erbsland::cryptology {

/// A copyable server credential containing only an Ed25519 public verifier.
/// Its `-s-` text is public verification data; it cannot answer a client challenge.
/// @seedoc{/reference/cryptology/authentication_tokens}
/// @tested{AuthenticationTokenTest}
class ServerAuthenticationToken final {
    friend class AuthenticationTokenPair;

public:
    /// Create an empty placeholder.
    ServerAuthenticationToken() = default;

    // defaults
    ~ServerAuthenticationToken() = default;
    ServerAuthenticationToken(const ServerAuthenticationToken &) = default;
    ServerAuthenticationToken(ServerAuthenticationToken &&) noexcept = default;
    auto operator=(const ServerAuthenticationToken &) -> ServerAuthenticationToken & = default;
    auto operator=(ServerAuthenticationToken &&) noexcept -> ServerAuthenticationToken & = default;

public: // main operations
    /// Create one fresh challenge with a positive lifetime of at most 60 seconds.
    /// @param lifetime The monotonic lifetime, defaulting to 60 seconds.
    /// @return One-use pending verification state.
    /// @throws err::ParameterError If the lifetime is outside the accepted range.
    [[nodiscard]] auto createChallenge(time::TimeDelta lifetime = time::Seconds{60}) const
        -> PendingAuthenticationChallenge;
    /// Check that this verifier matches a client credential, including its identifier.
    [[nodiscard]] auto matches(const ClientAuthenticationToken &client) const -> bool;

public: // tests and accessors
    /// Test whether this server verifier is empty.
    [[nodiscard]] auto isEmpty() const noexcept -> bool { return _key.isEmpty(); }
    /// Get the public identifier used to select this verifier.
    [[nodiscard]] auto id() const noexcept -> const text::String & { return _id; }

public: // conversion
    /// Return canonical public `elctk-<id>-s-<data>` text.
    [[nodiscard]] auto toString() const -> text::String;
    /// Parse server credential text, returning an empty placeholder on failure.
    [[nodiscard]] static auto fromString(const text::String &token) noexcept -> ServerAuthenticationToken;
    /// Parse server credential text.
    /// @throws err::ParseError If the format, role, public key, or encoding is invalid.
    [[nodiscard]] static auto fromStringOrThrow(const text::String &token) -> ServerAuthenticationToken;

private:
    /// Store a validated identifier and public key.
    ServerAuthenticationToken(text::String id, PublicKey key) noexcept;

private:
    text::String _id; ///< Public lookup identifier.
    PublicKey _key;   ///< Public verifier.
};

}
