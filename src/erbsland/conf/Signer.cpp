// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "Signer.hpp"

#include "impl/sign/Signer.hpp"

#include "../err/ParameterError.hpp"

#include <utility>

namespace erbsland::conf {

using namespace text::literals;

Signer::Signer(SignatureSignerPtr signatureSigner) : _signatureSigner{std::move(signatureSigner)} {
    if (_signatureSigner == nullptr) {
        throw err::ParameterError("Signature signer must not be null"_el, "signatureSigner"_el);
    }
}

void Signer::sign(path::Path sourcePath, path::Path destinationPath, text::String signingPersonText) {

    impl::Signer{_signatureSigner}.sign(
        std::move(sourcePath), std::move(destinationPath), std::move(signingPersonText));
}

}
