// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#include "KeyAgreementSharedSecret.hpp"

namespace erbsland::cryptology {

KeyAgreementSharedSecret::KeyAgreementSharedSecret(
    const KeyAgreementAlgorithm algorithm, const mem::ConstByteSpan data) :
    _algorithm{algorithm}, _data{data} {
}

KeyAgreementSharedSecret::~KeyAgreementSharedSecret() {
    secureErase();
}

void KeyAgreementSharedSecret::secureErase() noexcept {
    _data.secureErase();
}

}
