// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "AuthenticationTokenPair.hpp"

#include "../impl/AuthenticationCodec.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

AuthenticationTokenPair::AuthenticationTokenPair(
    ClientAuthenticationToken client, ServerAuthenticationToken server) noexcept :
    _client{std::move(client)}, _server{std::move(server)} {
}

auto AuthenticationTokenPair::generate(const text::String &id) -> AuthenticationTokenPair {
    if (!impl::authentication_codec::isValidIdentifier(id)) {
        throw err::ParameterError{"Authentication-token identifier must match [_a-z0-9]{1,64}."_el, "id"_el};
    }
    auto key = SigningPrivateKey::generate(SigningKeyProfile::Ed25519);
    auto verifier = key.publicKey();
    return AuthenticationTokenPair{
        ClientAuthenticationToken{id, std::move(key)}, ServerAuthenticationToken{id, std::move(verifier)}};
}

}
