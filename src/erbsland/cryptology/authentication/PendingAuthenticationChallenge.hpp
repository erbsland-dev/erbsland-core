// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "PendingAuthenticationChallenge_fwd.hpp"
#include "ServerAuthenticationToken_fwd.hpp"

#include "../impl/AuthenticationChallengeTestAccess_fwd.hpp"
#include "../keys/PublicKey.hpp"

#include "../../mem/ByteBlock.hpp"
#include "../../mem/ByteSpan.hpp"
#include "../../text/String.hpp"
#include "../../time/TimePoint.hpp"

namespace erbsland::cryptology {

/// One pending, one-use authentication attempt bound to one server credential.
/// An application must keep this state with the connection or request that issued its challenge.
/// @seedoc{/reference/cryptology/authentication_tokens}
/// @tested{AuthenticationTokenTest}
class PendingAuthenticationChallenge final {
    friend class ServerAuthenticationToken;
    friend class impl::AuthenticationChallengeTestAccess;

public:
    /// Create an empty placeholder.
    PendingAuthenticationChallenge() noexcept = default;

    // defaults/deletions
    ~PendingAuthenticationChallenge() = default;
    PendingAuthenticationChallenge(const PendingAuthenticationChallenge &) = delete;
    PendingAuthenticationChallenge(PendingAuthenticationChallenge &&) noexcept = default;
    auto operator=(const PendingAuthenticationChallenge &) -> PendingAuthenticationChallenge & = delete;
    auto operator=(PendingAuthenticationChallenge &&) noexcept -> PendingAuthenticationChallenge & = default;

public: // main operations
    /// Verify a signed response once, consuming this attempt even when verification fails.
    /// @param response The complete serialized signed response.
    /// @return `true` only for this challenge, credential, purpose, and an unexpired deadline.
    [[nodiscard]] auto verify(mem::ConstByteSpan response) -> bool;

public: // tests and accessors
    /// Test whether the pending attempt is empty or has already been used.
    [[nodiscard]] auto isConsumed() const noexcept -> bool { return _consumed || _challenge.isEmpty(); }
    /// Get the challenge bytes to send to the client.
    [[nodiscard]] auto challengeBytes() const noexcept -> mem::ByteBlock { return _challenge; }

private:
    /// Store a freshly issued challenge and its monotonic deadline.
    PendingAuthenticationChallenge(
        text::String id, PublicKey key, mem::ByteBlock challenge, time::TimePoint deadline) noexcept;
    /// Verify using an explicit clock value for deterministic internal tests.
    [[nodiscard]] auto verifyAt(mem::ConstByteSpan response, time::TimePoint now) -> bool;

private:
    text::String _id;          ///< Expected public client identifier.
    PublicKey _key;            ///< Expected server-side verifier.
    mem::ByteBlock _challenge; ///< Exact issued challenge bytes.
    time::TimePoint _deadline; ///< Monotonic expiry point.
    bool _consumed{false};     ///< One-use state.
};

}
