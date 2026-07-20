// Copyright (c) 2025 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <cstdint>

namespace erbsland::conf {

/// The result of the signature validation.
enum class SignatureValidatorResult : uint8_t {
    /// The signature is correct. Accept the document.
    Accept,
    /// The signature is not correct. Reject the document.
    Reject,
};

}
