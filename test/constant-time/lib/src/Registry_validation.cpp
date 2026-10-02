// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0

#include "Registry.hpp"

#include "cases/validation/ValidationCase.hpp"

namespace app::constant_time {

using namespace el::text::literals;

void Registry::addValidation() {
    for (
        const auto operation :
        {ValidationOperation::Aes128Gcm,
            ValidationOperation::Aes256Gcm,
            ValidationOperation::ChaCha20Poly1305,
            ValidationOperation::CbcPadding}) {
        add<ValidationCase>(operation);
    }
}

}
