// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "ClientAuthenticationToken.hpp"

#include "../impl/AuthenticationCodec.hpp"
#include "../SignedByteBlock.hpp"

#include "../../err/LogicError.hpp"
#include "../../err/ParseError.hpp"
#include "../../err/RuntimeError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

ClientAuthenticationToken::ClientAuthenticationToken(text::String id, SigningPrivateKey key) noexcept :
    _id{std::move(id)}, _key{std::move(key)} {
}

auto ClientAuthenticationToken::createResponse(const mem::ConstByteSpan challenge) const -> SignedByteBlock {
    if (isEmpty()) {
        throw err::LogicError{"Cannot answer a challenge with an empty client token."_el};
    }
    if (impl::authentication_codec::decodeChallengeIdentifier(challenge) != _id) {
        throw err::ParseError{"Authentication challenge addresses a different client identifier."_el};
    }
    return SignedByteBlock::sign(_key, challenge, "erbsland.core.auth-token.challenge.v1"_el, _id);
}

auto ClientAuthenticationToken::toString() const -> text::String {
    if (isEmpty()) {
        throw err::LogicError{"Cannot encode an empty client token."_el};
    }
    auto result = text::String{};
    _key._privateData.withUnprotectedData([&](const mem::ConstByteSpan seed) -> void {
        result = impl::authentication_codec::encodeToken(_id, 'c', seed);
    });
    return result;
}

auto ClientAuthenticationToken::fromString(const text::String &token) noexcept -> ClientAuthenticationToken {
    try {
        return fromStringOrThrow(token);
    } catch (const err::RuntimeError &) {
        return {};
    }
}

auto ClientAuthenticationToken::fromStringOrThrow(const text::String &token) -> ClientAuthenticationToken {
    auto sensitiveText = token;
    sensitiveText.markAsSensitive();
    auto identifier = text::String{};
    auto seed = impl::authentication_codec::decodeToken(sensitiveText, 'c', identifier);
    seed.markAsSensitive();
    auto key = SigningPrivateKey::fromEd25519Seed(seed.span());
    seed.secureErase();
    return ClientAuthenticationToken{std::move(identifier), std::move(key)};
}

}
