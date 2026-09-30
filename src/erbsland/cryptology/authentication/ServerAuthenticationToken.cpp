// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ServerAuthenticationToken.hpp"

#include "ClientAuthenticationToken.hpp"
#include "PendingAuthenticationChallenge.hpp"

#include "../impl/algorithm/ed25519_signature/Ed25519Signature.hpp"
#include "../impl/AuthenticationCodec.hpp"
#include "../impl/DerEncoder.hpp"
#include "../impl/SigningKeyEncoding.hpp"

#include "../../core/Application.hpp"
#include "../../err/LogicError.hpp"
#include "../../err/ParameterError.hpp"
#include "../../err/ParseError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../random/Random.hpp"
#include "../../text/Literals.hpp"
#include "../../unit/ByteLength.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

ServerAuthenticationToken::ServerAuthenticationToken(text::String id, PublicKey key) noexcept :
    _id{std::move(id)}, _key{std::move(key)} {
}

auto ServerAuthenticationToken::createChallenge(const time::TimeDelta lifetime) const
    -> PendingAuthenticationChallenge {
    if (isEmpty()) {
        throw err::LogicError{"Cannot create a challenge from an empty server token."_el};
    }
    if (!lifetime.isPositive() || lifetime > time::TimeDelta{time::Seconds{60}}) {
        throw err::ParameterError{"Challenge lifetime must be positive and at most 60 seconds."_el, "lifetime"_el};
    }
    const auto start = time::TimePoint::now();
    const auto nonce = core::application().secureRandom().buildByteBlock(unit::ByteLength{32U});
    const auto challenge = impl::authentication_codec::encodeChallenge(_id, nonce.span());
    return PendingAuthenticationChallenge{_id, _key, challenge, start + lifetime};
}

auto ServerAuthenticationToken::matches(const ClientAuthenticationToken &client) const -> bool {
    return !isEmpty() && !client.isEmpty() && _id == client._id && client._key.matches(_key);
}

auto ServerAuthenticationToken::toString() const -> text::String {
    if (isEmpty()) {
        throw err::LogicError{"Cannot encode an empty server token."_el};
    }
    return impl::authentication_codec::encodeToken(_id, 's', _key.keyData().span());
}

auto ServerAuthenticationToken::fromString(const text::String &token) noexcept -> ServerAuthenticationToken {
    try {
        return fromStringOrThrow(token);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto ServerAuthenticationToken::fromStringOrThrow(const text::String &token) -> ServerAuthenticationToken {
    auto identifier = text::String{};
    const auto publicBytes = impl::authentication_codec::decodeToken(token, 's', identifier);
    auto encoder = impl::DerEncoder{};
    impl::signing_key_encoding::appendEd25519PublicKey(encoder, publicBytes.span());
    auto key = PublicKey::fromDerOrThrow(encoder.encoded());
    const auto point = impl::ed25519_signature::decodePoint(publicBytes.span());
    if (!point.has_value() || !impl::ed25519_signature::isPrimeOrderPoint(*point)) {
        throw err::ParseError{"Server authentication token contains an invalid Ed25519 public key."_el};
    }
    return ServerAuthenticationToken{std::move(identifier), std::move(key)};
}

}
