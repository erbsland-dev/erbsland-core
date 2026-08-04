// Copyright (c) 2026 Tobias Erbsland - https://erbsland.dev
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "ProtectedDataAccess.hpp"

namespace erbsland::cryptology::impl {

/// Forwards protected-data operations into the application configuration.
/// @notest{Exercised through ProtectedByteBlockTest.}
class ApplicationProtectedDataAccess final : public ProtectedDataAccess {
public:
    [[nodiscard]] auto protect(mem::ConstByteSpan plaintext, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;
    [[nodiscard]] auto unprotect(mem::ConstByteSpan envelope, unit::ByteLength plaintextLength)
        -> mem::ByteBlock override;
};

}
