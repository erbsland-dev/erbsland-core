// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "RsaContext.hpp"

#include <erbsland/core/Application.hpp>
#include <erbsland/cryptology/impl/DerParser.hpp>
#include <erbsland/cryptology/impl/PrivateKeyParser.hpp>
#include <erbsland/cryptology/keys/SigningPrivateKey.hpp>
#include <erbsland/resource/Resources.hpp>
#include <erbsland/text/String.hpp>
#include <erbsland/unit/ItemIndex.hpp>

namespace app::constant_time {

using namespace el::text::literals;

RsaContext::RsaContext() {
    namespace ci = erbsland::cryptology::impl;
    const auto pem = el::application().resources().getTextOrThrow("constant-time-fixtures"_el, "rsa-2048-pkcs8.pem"_el);
    const auto key = el::SigningPrivateKey::fromPemOrThrow(pem);
    const auto der = ci::PrivateKeyParser::decodePem(pem);
    const auto node = ci::DerParser{der}.parseDocument();
    _privateKey = node.child(el::ItemIndex{2}).contentData();
    _publicKey = key.publicKey();
    _components = ci::rsa_signer::decodePrivateKey(_privateKey.span());
}

}
