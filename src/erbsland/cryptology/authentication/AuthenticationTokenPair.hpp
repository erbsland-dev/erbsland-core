// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ClientAuthenticationToken.hpp"
#include "ServerAuthenticationToken.hpp"

#include "../../text/String.hpp"

namespace erbsland::cryptology {

/// A newly generated client credential and matching public server verifier.
/// @seedoc{/reference/cryptology/authentication_tokens}
/// @tested{AuthenticationTokenTest}
class AuthenticationTokenPair final {
public:
    // defaults/deletions
    ~AuthenticationTokenPair() = default;
    AuthenticationTokenPair(const AuthenticationTokenPair &) = delete;
    AuthenticationTokenPair(AuthenticationTokenPair &&) noexcept = default;
    auto operator=(const AuthenticationTokenPair &) -> AuthenticationTokenPair & = delete;
    auto operator=(AuthenticationTokenPair &&) noexcept -> AuthenticationTokenPair & = default;

public: // accessors
    /// Get the client credential, which may be moved out of this pair.
    [[nodiscard]] auto client() noexcept -> ClientAuthenticationToken & { return _client; }
    /// Get the client credential without moving it.
    [[nodiscard]] auto client() const noexcept -> const ClientAuthenticationToken & { return _client; }
    /// Get the server verifier, which may be copied or moved out of this pair.
    [[nodiscard]] auto server() noexcept -> ServerAuthenticationToken & { return _server; }
    /// Get the server verifier without modifying it.
    [[nodiscard]] auto server() const noexcept -> const ServerAuthenticationToken & { return _server; }

public: // factories
    /// Generate a new Ed25519 pair from secure randomness and an exact public identifier.
    /// @param id An identifier matching `[_a-z0-9]{1,64}`.
    /// @return Matching client and server credentials.
    /// @throws err::ParameterError If the identifier is invalid.
    [[nodiscard]] static auto generate(const text::String &id) -> AuthenticationTokenPair;

private:
    /// Store one matching pair.
    AuthenticationTokenPair(ClientAuthenticationToken client, ServerAuthenticationToken server) noexcept;

private:
    ClientAuthenticationToken _client; ///< Protected client credential.
    ServerAuthenticationToken _server; ///< Public server verifier.
};

}
