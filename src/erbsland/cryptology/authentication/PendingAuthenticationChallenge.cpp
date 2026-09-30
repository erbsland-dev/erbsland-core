// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "PendingAuthenticationChallenge.hpp"

#include "../SignedByteBlock.hpp"

#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

PendingAuthenticationChallenge::PendingAuthenticationChallenge(
    text::String id, PublicKey key, mem::ByteBlock challenge, const time::TimePoint deadline) noexcept :
    _id{std::move(id)}, _key{std::move(key)}, _challenge{std::move(challenge)}, _deadline{deadline} {
}

auto PendingAuthenticationChallenge::verify(const mem::ConstByteSpan response) -> bool {
    return verifyAt(response, time::TimePoint::now());
}

auto PendingAuthenticationChallenge::verifyAt(const mem::ConstByteSpan response, const time::TimePoint now) -> bool {
    if (isConsumed()) {
        return false;
    }
    _consumed = true;
    if (now > _deadline) {
        return false;
    }
    const auto signedResponse = SignedByteBlock::fromByteBlock(response, unit::ByteLength{102U});
    if (signedResponse.isEmpty() || signedResponse.untrustedKeyIdHint() != _id) {
        return false;
    }
    const auto payload = signedResponse.verify(_key, "erbsland.core.auth-token.challenge.v1"_el);
    return payload.has_value() && *payload == _challenge;
}

}
