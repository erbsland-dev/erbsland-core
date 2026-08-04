// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementPublicKey.hpp"

#include "../../err/ParameterError.hpp"
#include "../../text/Literals.hpp"

namespace erbsland::cryptology {

using namespace text::literals;

KeyAgreementPublicKey::KeyAgreementPublicKey(const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan data) :
    _algorithm{algorithm}, _data{mem::ByteBlock::fromSpan(data)} {
    if (algorithm.publicKeySize().isZero() || data.size() != algorithm.publicKeySize().toSizeT()) {
        throw err::ParameterError{"The public-key length does not match the key-agreement algorithm."_el, "data"_el};
    }
}

}
