// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <DemoCommon.hpp>
#include <erbsland/conf/SignatureValidator.hpp>
#include <erbsland/cryptology/Hmac.hpp>
#include <erbsland/text/base_n/BaseNDecoder.hpp>
#include <erbsland/text/base_n/BaseNFormat.hpp>

namespace demo {

/// Validate configuration signatures with a trusted signer label and shared HMAC key.
///
/// The parser calls `validate()` for every source, even when its signature text is empty. The policy can therefore
/// require signatures for deployed files or admit unsigned files in a controlled development mode. Signed input is
/// accepted only when its format, signer label, and constant-time HMAC verification all succeed.
class HmacSignatureValidator final : public el::conf::SignatureValidator {
public:
    enum class UnsignedDocuments : uint8_t {
        Reject,
        Accept,
    };

public:
    HmacSignatureValidator(
        el::ByteBlock key,
        el::String trustedSigner,
        const UnsignedDocuments unsignedDocuments = UnsignedDocuments::Reject) :
        _key{std::move(key)}, _trustedSigner{std::move(trustedSigner)}, _unsignedDocuments{unsignedDocuments} {
        _key.markAsSensitive();
    }

public: // implement `SignatureValidator`
    [[nodiscard]] auto validate(const el::conf::SignatureValidatorData &data)
        -> el::conf::SignatureValidatorResult override {
        using Result = el::conf::SignatureValidatorResult;
        if (data.signatureText.isEmpty()) {
            return _unsignedDocuments == UnsignedDocuments::Accept ? Result::Accept : Result::Reject;
        }

        const auto algorithmSeparator = data.signatureText.find(";"_el);
        if (algorithmSeparator.isNoIndex()) {
            return Result::Reject;
        }
        auto [algorithm, payload] = data.signatureText.splitAt(algorithmSeparator);
        payload = std::get<1>(payload.slice(el::StringSide::Front));

        const auto signerSeparator = payload.find(";"_el);
        if (signerSeparator.isNoIndex()) {
            return Result::Reject;
        }
        auto [signer, authenticatorText] = payload.splitAt(signerSeparator);
        authenticatorText = std::get<1>(authenticatorText.slice(el::StringSide::Front));
        if (algorithm != "hmac-sha256"_el || signer != _trustedSigner) {
            return Result::Reject;
        }

        const auto expected =
            el::base_n::BaseNDecoder{authenticatorText, el::base_n::BaseNFormat::base64()}.toData(el::ByteLength{32U});
        if (!expected.has_value()) {
            return Result::Reject;
        }
        auto authenticator = el::cryptology::Hmac{el::HashAlgorithm::Sha2_256, _key};
        authenticator.update(signer);
        authenticator.update("\n"_el);
        authenticator.update(data.documentDigest);
        return authenticator.verify(expected.value()) ? Result::Accept : Result::Reject;
    }

private:
    el::ByteBlock _key;
    el::String _trustedSigner;
    UnsignedDocuments _unsignedDocuments;
};

}
