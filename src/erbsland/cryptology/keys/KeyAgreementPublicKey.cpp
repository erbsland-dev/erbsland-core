// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementPublicKey.hpp"

#include "../impl/key_agreement/P256KeyAgreement.hpp"

#include "../../err/ParameterFailure.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

KeyAgreementPublicKey::KeyAgreementPublicKey(const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan data) :
    _algorithm{algorithm}, _data{mem::ByteBlock::fromSpan(data)} {
    if (algorithm.publicKeySize().isZero() || data.size() != algorithm.publicKeySize().toSizeT()) {
        throw err::ParameterFailure{"The public-key length does not match the key-agreement algorithm."_el, "data"_el};
    }
    if (algorithm == KeyAgreementAlgorithm::P256 && !impl::p256_key_agreement::isValidPublicKey(data)) {
        throw err::ParameterFailure{"The P-256 public key must be a valid uncompressed point."_el, "data"_el};
    }
}

}
