// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "../err/RuntimeError.hpp"

namespace erbsland::cryptology {

/// An error reported by a cryptographic operation or backend.
/// @tested{SymmetricEncryptionFrontendTest}
class CryptologyError final : public err::RuntimeError {
public:
    using err::RuntimeError::RuntimeError;

    // defaults
    ~CryptologyError() override = default;
};

}
