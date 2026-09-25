// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/conf/ConfError.hpp>
#include <erbsland/conf/SignatureSigner.hpp>
#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/text/base_n/BaseNEncoder.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>

namespace demo {

/// Create configuration signature text with a shared HMAC key.
///
/// `SignatureSigner` receives the document digest in exactly the representation expected by the parser. This example
/// authenticates the signer label and digest together, then stores the algorithm, label, and Base64 authenticator in
/// the
/// `@signature` text. A production backend can use the same interface with a private key, certificate, or signing
/// service instead.
class HmacSignatureSigner final : public el::conf::SignatureSigner {
public:
    explicit HmacSignatureSigner(el::ByteBlock key) : _key{std::move(key)} { _key.markAsSensitive(); }

public: // implement `SignatureSigner`
    [[nodiscard]] auto sign(const el::conf::SignatureSignerData &data) -> el::String override {
        if (data.signingPersonText.isEmpty() || data.signingPersonText.contains(";"_el)) {
            throw el::conf::ConfError{
                el::conf::ConfErrorCategory::Signature,
                "The signer label must be non-empty and must not contain a semicolon."_el};
        }

        auto authenticator = el::cryptology::Hmac{el::HashAlgorithm::Sha2_256, _key};
        authenticator.update(data.signingPersonText);
        authenticator.update("\n"_el);
        authenticator.update(data.documentDigest);
        const auto encoded =
            el::base_n::BaseNEncoder{authenticator.finalize(), el::base_n::BaseNFormat::base64()}.toString();
        return el::String::fromJoined({"hmac-sha256;"_el, data.signingPersonText, ";"_el, encoded});
    }

private:
    el::ByteBlock _key;
};

}
